// Métro pour Windows — fenêtre native qui affiche l'application (dossier app/),
// avec WebView2, le moteur d'Edge présent dans Windows 10/11.
//
//   - la page (Metro.html, un seul fichier) est embarquée dans l'exécutable et
//     servie depuis https://app.metro.example/ (origine fixe : les réglages,
//     gardés par la page, sont conservés d'une fois sur l'autre) ;
//   - barre de titre sombre, icône, position et taille de la fenêtre mémorisées ;
//   - fonctions appelées par la page : premier plan, mode compact, liens externes.
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "WebView2.h"
#include "webview/webview.h"

static const wchar_t* HOST = L"app.metro.example";
static const int ID_PAGE = 100;  // ressource : la page (metro.rc)

static HWND win = nullptr;
static ICoreWebView2Controller* controller = nullptr;
static bool compact = false, onTop = false, startCompact = false;
static RECT normalRect{}, compactRect{};
static bool maximized = false;
static std::wstring dataDir;

// ---- Fichiers ---------------------------------------------------------------------------------
static std::wstring localAppData() {
  wchar_t p[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, p))) return p;
  return L".";
}

static bool writeIfChanged(const std::wstring& path, const void* data, DWORD size) {
  HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER sz;
    bool same = GetFileSizeEx(f, &sz) && sz.QuadPart == size;
    if (same) {
      std::string cur(size, '\0');
      DWORD got = 0;
      same = ReadFile(f, &cur[0], size, &got, nullptr) && got == size && memcmp(cur.data(), data, size) == 0;
    }
    CloseHandle(f);
    if (same) return true;
  }
  f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) return false;
  DWORD w = 0;
  BOOL ok = WriteFile(f, data, size, &w, nullptr);
  CloseHandle(f);
  return ok && w == size;
}

// La page embarquée est recopiée dans %LOCALAPPDATA%\Metro\app\index.html
static bool extractPage(const std::wstring& dir) {
  HRSRC r = FindResourceW(nullptr, MAKEINTRESOURCEW(ID_PAGE), MAKEINTRESOURCEW(10 /* RT_RCDATA */));
  if (!r) return false;
  HGLOBAL g = LoadResource(nullptr, r);
  const void* data = g ? LockResource(g) : nullptr;
  DWORD size = SizeofResource(nullptr, r);
  if (!data || !size) return false;
  CreateDirectoryW(dir.c_str(), nullptr);
  return writeIfChanged(dir + L"\\index.html", data, size);
}

// ---- Fenêtre : position, taille, mode compact ---------------------------------------------------------
static int dpiScale(int v) {
  UINT dpi = 96;
  HMODULE u = GetModuleHandleW(L"user32.dll");
  auto getDpi = u ? (UINT(WINAPI*)(HWND))GetProcAddress(u, "GetDpiForWindow") : nullptr;
  if (getDpi && win) dpi = getDpi(win);
  return MulDiv(v, (int)dpi, 96);
}

static std::wstring stateFile() { return dataDir + L"\\fenetre.txt"; }

static bool rectVisible(const RECT& r) {  // le rectangle touche-t-il un écran branché ?
  return r.right - r.left > 200 && r.bottom - r.top > 150 && MonitorFromRect(&r, MONITOR_DEFAULTTONULL) != nullptr;
}

static void loadState() {
  FILE* f = _wfopen(stateFile().c_str(), L"r");
  if (!f) return;
  int c = 0, t = 0, m = 0;
  RECT n{}, k{};
  if (fscanf(f, "%ld %ld %ld %ld %d %ld %ld %ld %ld %d %d", &n.left, &n.top, &n.right, &n.bottom, &m, &k.left, &k.top,
             &k.right, &k.bottom, &c, &t) >= 5) {
    if (rectVisible(n)) normalRect = n;
    if (rectVisible(k)) compactRect = k;
    maximized = m != 0;
    startCompact = c != 0 && rectVisible(compactRect);
  }
  fclose(f);
}

static void saveState() {
  if (!win) return;
  WINDOWPLACEMENT wp{sizeof(wp)};
  if (GetWindowPlacement(win, &wp)) {
    if (compact) compactRect = wp.rcNormalPosition;
    else {
      normalRect = wp.rcNormalPosition;
      maximized = wp.showCmd == SW_SHOWMAXIMIZED;
    }
  }
  FILE* f = _wfopen(stateFile().c_str(), L"w");
  if (!f) return;
  fprintf(f, "%ld %ld %ld %ld %d %ld %ld %ld %ld %d %d\n", normalRect.left, normalRect.top, normalRect.right,
          normalRect.bottom, maximized ? 1 : 0, compactRect.left, compactRect.top, compactRect.right, compactRect.bottom,
          compact ? 1 : 0, onTop ? 1 : 0);
  fclose(f);
}

static void place(const RECT& r) {
  SetWindowPos(win, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
}

static void setCompact(bool on) {
  if (on == compact) return;
  WINDOWPLACEMENT wp{sizeof(wp)};
  GetWindowPlacement(win, &wp);
  if (on) {
    normalRect = wp.rcNormalPosition;
    maximized = wp.showCmd == SW_SHOWMAXIMIZED;
    if (maximized) ShowWindow(win, SW_RESTORE);
    if (!rectVisible(compactRect)) {  // première fois : en haut à droite de l'écran
      MONITORINFO mi{sizeof(mi)};
      GetMonitorInfoW(MonitorFromWindow(win, MONITOR_DEFAULTTONEAREST), &mi);
      const int w = dpiScale(480), h = dpiScale(420), m = dpiScale(16);
      compactRect = {mi.rcWork.right - w - m, mi.rcWork.top + m, mi.rcWork.right - m, mi.rcWork.top + m + h};
    }
    compact = true;
    place(compactRect);
  } else {
    compactRect = wp.rcNormalPosition;
    compact = false;
    if (rectVisible(normalRect)) place(normalRect);
    if (maximized) ShowWindow(win, SW_MAXIMIZE);
  }
  saveState();
}

static void setOnTop(bool on) {
  onTop = on;
  SetWindowPos(win, on ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static LRESULT CALLBACK subclass(HWND h, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
  if (msg == WM_CLOSE || msg == WM_EXITSIZEMOVE) saveState();
  // Fenêtre réduite : la page passe en arrière-plan (plus d'appels à l'API)
  if (msg == WM_SIZE && controller) controller->put_IsVisible(wp == SIZE_MINIMIZED ? FALSE : TRUE);
  if (msg == WM_GETMINMAXINFO) {  // taille minimale : celle du mode compact
    auto* mm = (MINMAXINFO*)lp;
    mm->ptMinTrackSize.x = dpiScale(360);
    mm->ptMinTrackSize.y = dpiScale(260);
  }
  return DefSubclassProc(h, msg, wp, lp);
}

// Barre de titre assortie à l'application (Windows 11 ; ignoré ailleurs)
static void darkTitleBar() {
  BOOL dark = TRUE;
  DwmSetWindowAttribute(win, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
  COLORREF caption = RGB(0x0a, 0x0f, 0x19), text = RGB(0xf3, 0xf5, 0xf9), border = RGB(0x1a, 0x23, 0x32);
  DwmSetWindowAttribute(win, 35 /* DWMWA_CAPTION_COLOR */, &caption, sizeof(caption));
  DwmSetWindowAttribute(win, 36 /* DWMWA_TEXT_COLOR */, &text, sizeof(text));
  DwmSetWindowAttribute(win, 34 /* DWMWA_BORDER_COLOR */, &border, sizeof(border));
}

// ---- Appels depuis la page : metroNative(commande, valeur) ------------------------------------------
// Requête de la page : ["commande", valeur] (JSON)
static std::string trim(std::string s);
static void parseRequest(const std::string& req, std::string& cmd, std::string& arg) {
  const size_t a = req.find('"');
  const size_t b = a == std::string::npos ? a : req.find('"', a + 1);
  if (b == std::string::npos) return;
  cmd = req.substr(a + 1, b - a - 1);
  const size_t c = req.find(',', b), e = req.rfind(']');
  if (c != std::string::npos && e != std::string::npos && e > c) arg = trim(req.substr(c + 1, e - c - 1));
}

static std::string trim(std::string s) {
  while (!s.empty() && isspace((unsigned char)s.front())) s.erase(0, 1);
  while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
  return s;
}

static std::wstring widen(const std::string& s) {
  int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  std::wstring w(n > 0 ? n - 1 : 0, L'\0');
  if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
  return w;
}

static std::string nativeCall(const std::string& req) {
  std::string cmd, arg;
  parseRequest(req, cmd, arg);
  if (cmd == "ontop") setOnTop(arg == "true");
  else if (cmd == "compact") setCompact(arg == "true");
  else if (cmd == "open" && arg.size() > 2 && arg.front() == '"' && arg.back() == '"') {
    std::string url = arg.substr(1, arg.size() - 2);  // adresse web seulement
    if (url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0)
      ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  }
  return "true";
}

// ---- Démarrage -----------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
  // Une seule fenêtre : un second lancement ramène la première au premier plan
  HANDLE single = CreateMutexW(nullptr, TRUE, L"Local\\MetroPourWindows");
  if (single && GetLastError() == ERROR_ALREADY_EXISTS) {
    HWND other = FindWindowW(L"webview", L"Métro");
    if (other) {
      ShowWindow(other, SW_RESTORE);
      SetForegroundWindow(other);
    }
    return 0;
  }

  dataDir = localAppData() + L"\\Metro";
  CreateDirectoryW(dataDir.c_str(), nullptr);
  const std::wstring pageDir = dataDir + L"\\app";
  if (!extractPage(pageDir)) {
    MessageBoxW(nullptr, L"Impossible de préparer l'application (dossier %LOCALAPPDATA%\\Metro).", L"Métro", MB_ICONERROR);
    return 1;
  }
  // Données de WebView2 (réglages de la page, cache) : à côté, plutôt qu'à côté de l'exécutable
  SetEnvironmentVariableW(L"WEBVIEW2_USER_DATA_FOLDER", (dataDir + L"\\WebView2").c_str());
  loadState();

  try {
    webview::webview w(false, nullptr);
    win = (HWND)w.window().value();
    SetWindowTextW(win, L"Métro");
    HICON big = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    HICON small = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    if (big) SendMessageW(win, WM_SETICON, ICON_BIG, (LPARAM)big);
    if (small) SendMessageW(win, WM_SETICON, ICON_SMALL, (LPARAM)small);
    darkTitleBar();
    SetWindowSubclass(win, subclass, 1, 0);

    // Taille et position : celles de la dernière fois, sinon 1360 x 860 au centre
    if (startCompact) {  // fermée en mode compact : rouverte ainsi
      compact = true;
      place(compactRect);
    } else if (rectVisible(normalRect)) place(normalRect);
    else {
      MONITORINFO mi{sizeof(mi)};
      GetMonitorInfoW(MonitorFromWindow(win, MONITOR_DEFAULTTOPRIMARY), &mi);
      const int ww = std::min<int>(dpiScale(1360), mi.rcWork.right - mi.rcWork.left - 40);
      const int hh = std::min<int>(dpiScale(860), mi.rcWork.bottom - mi.rcWork.top - 40);
      const int x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - ww) / 2;
      const int y = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - hh) / 2;
      SetWindowPos(win, nullptr, x, y, ww, hh, SWP_NOZORDER);
    }
    if (maximized && !compact) ShowWindow(win, SW_MAXIMIZE);

    // Page servie par WebView2 depuis le dossier local, sous une adresse fixe
    controller = (ICoreWebView2Controller*)w.browser_controller().value();
    ICoreWebView2* core = nullptr;
    if (controller && SUCCEEDED(controller->get_CoreWebView2(&core)) && core) {
      ICoreWebView2_3* core3 = nullptr;
      if (SUCCEEDED(core->QueryInterface(IID_ICoreWebView2_3, (void**)&core3)) && core3) {
        core3->SetVirtualHostNameToFolderMapping(HOST, pageDir.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
        core3->Release();
      }
      ICoreWebView2Settings* s = nullptr;
      if (SUCCEEDED(core->get_Settings(&s)) && s) {
        s->put_IsStatusBarEnabled(FALSE);
        s->put_AreDevToolsEnabled(FALSE);
        s->Release();
      }
      core->Release();
    }
    ICoreWebView2Controller2* c2 = nullptr;
    if (controller && SUCCEEDED(controller->QueryInterface(IID_ICoreWebView2Controller2, (void**)&c2)) && c2) {
      COREWEBVIEW2_COLOR bg{255, 0x05, 0x07, 0x0c};  // pas d'éclair blanc au démarrage
      c2->put_DefaultBackgroundColor(bg);
      c2->Release();
    }

    w.bind("metroNative", nativeCall);
    w.navigate("https://app.metro.example/index.html");
    w.run();
  } catch (const std::exception&) {
    if (MessageBoxW(nullptr,
                    L"Le moteur WebView2 de Microsoft est introuvable sur cet ordinateur.\n\n"
                    L"Il est inclus dans Windows 11 ; sinon, installez « Microsoft Edge WebView2 Runtime » "
                    L"(gratuit). Ouvrir la page de téléchargement ?",
                    L"Métro", MB_ICONWARNING | MB_YESNO) == IDYES)
      ShellExecuteW(nullptr, L"open", L"https://developer.microsoft.com/microsoft-edge/webview2/", nullptr, nullptr,
                    SW_SHOWNORMAL);
    return 1;
  }
  return 0;
}
