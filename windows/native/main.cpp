// KeoLive pour Windows — le lanceur : une fenêtre native qui affiche l'application avec
// WebView2, le moteur d'Edge présent dans Windows 10/11.
//
//   - la page (KeoLive.html, un seul fichier) est posée à côté de KeoLive.exe. Le lanceur
//     la lit et la sert depuis la mémoire, sous https://app.metro.example/ (origine fixe :
//     les réglages, gardés par la page, sont conservés d'une fois sur l'autre).
//     Les nouvelles versions de l'interface ne changent que KeoLive.html : l'exécutable,
//     lui, reste le même fichier (même empreinte), ce qui évite qu'un antivirus le
//     découvre comme un programme inconnu à chaque mise à jour.
//     Rien n'est écrit sur le disque à part la position de la fenêtre ;
//   - barre de titre sombre, icône, position et taille de la fenêtre mémorisées ;
//   - fonctions appelées par la page : premier plan, mode compact, liens externes.
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "WebView2.h"
#include "webview/webview.h"

static const wchar_t* HOST = L"app.metro.example";
static const char* LAUNCHER_VERSION = "2.0.0";  // version du lanceur (metro.rc), pas de l'interface

static HWND win = nullptr;
static ICoreWebView2Controller* controller = nullptr;
static bool compact = false, onTop = false, startCompact = false, fullscreen = false;
static LONG_PTR savedStyle = 0, savedExStyle = 0;            // fenêtre avant le plein écran
static WINDOWPLACEMENT savedPlacement{sizeof(WINDOWPLACEMENT)};
static RECT normalRect{}, compactRect{};
static bool maximized = false;
static std::wstring dataDir;

// ---- Dossier des réglages ------------------------------------------------------------------------
static std::wstring localAppData() {
  wchar_t p[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, p))) return p;
  return L".";
}

// ---- La page : KeoLive.html, à côté de l'exécutable, servie depuis la mémoire -----------------------
// WebView2 demande https://app.metro.example/index.html ; le lanceur lui répond avec le contenu de
// KeoLive.html, relu à chaque chargement (F5 suffit après avoir remplacé le fichier).
static std::wstring pagePath;
static std::string page;  // dernier contenu lu

static std::wstring exeDir() {
  wchar_t p[MAX_PATH];
  const DWORD n = GetModuleFileNameW(nullptr, p, MAX_PATH);
  std::wstring s(p, n < MAX_PATH ? n : 0);
  const size_t k = s.find_last_of(L"\\/");
  return k == std::wstring::npos ? std::wstring(L".") : s.substr(0, k);
}

static bool readPage() {
  HANDLE f = CreateFileW(pagePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) return false;
  LARGE_INTEGER size{};
  bool ok = GetFileSizeEx(f, &size) && size.QuadPart > 0 && size.QuadPart < 64LL * 1024 * 1024;
  std::string buf;
  if (ok) {
    buf.resize((size_t)size.QuadPart);
    DWORD got = 0;
    ok = ReadFile(f, &buf[0], (DWORD)buf.size(), &got, nullptr) && got == buf.size();
  }
  CloseHandle(f);
  if (ok) page.swap(buf);
  return ok;
}

class PageServer final : public ICoreWebView2WebResourceRequestedEventHandler {
 public:
  explicit PageServer(ICoreWebView2Environment* env) : env_(env) { env_->AddRef(); }

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override {
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ICoreWebView2WebResourceRequestedEventHandler)) {
      *out = static_cast<ICoreWebView2WebResourceRequestedEventHandler*>(this);
      AddRef();
      return S_OK;
    }
    *out = nullptr;
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() override { return (ULONG)InterlockedIncrement(&refs_); }
  ULONG STDMETHODCALLTYPE Release() override {
    const LONG n = InterlockedDecrement(&refs_);
    if (n == 0) {
      env_->Release();
      delete this;
    }
    return (ULONG)n;
  }

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) override {
    std::wstring path;
    ICoreWebView2WebResourceRequest* req = nullptr;
    if (SUCCEEDED(args->get_Request(&req)) && req) {
      LPWSTR uri = nullptr;
      if (SUCCEEDED(req->get_Uri(&uri)) && uri) {
        path = uri;
        CoTaskMemFree(uri);
      }
      req->Release();
    }
    // « https://app.metro.example/index.html?… » -> « /index.html »
    const size_t h = path.find(HOST);
    path = h == std::wstring::npos ? std::wstring(L"?") : path.substr(h + wcslen(HOST));
    path = path.substr(0, path.find_first_of(L"?#"));
    const bool isPage = path.empty() || path == L"/" || path == L"/index.html";
    if (isPage) readPage();  // fichier remplacé entre-temps : nouvelle version ; illisible : la précédente

    IStream* body = isPage ? SHCreateMemStream((const BYTE*)page.data(), (UINT)page.size()) : nullptr;
    ICoreWebView2WebResourceResponse* resp = nullptr;
    if (SUCCEEDED(env_->CreateWebResourceResponse(body, isPage ? 200 : 404, isPage ? L"OK" : L"Not Found",
                                                  isPage ? L"Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store"
                                                       : L"Content-Type: text/plain",
                                                  &resp)) &&
        resp) {
      args->put_Response(resp);
      resp->Release();
    }
    if (body) body->Release();
    return S_OK;
  }

 private:
  ICoreWebView2Environment* env_;
  LONG refs_ = 1;
};

// Branche la page sur la vue ; faux si le moteur WebView2 est trop ancien
static bool servePage(ICoreWebView2* core) {
  bool ok = false;
  ICoreWebView2_2* core2 = nullptr;
  if (SUCCEEDED(core->QueryInterface(IID_ICoreWebView2_2, (void**)&core2)) && core2) {
    ICoreWebView2Environment* env = nullptr;
    if (SUCCEEDED(core2->get_Environment(&env)) && env) {
      const std::wstring filter = std::wstring(L"https://") + HOST + L"/*";
      if (SUCCEEDED(core->AddWebResourceRequestedFilter(filter.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL))) {
        PageServer* server = new PageServer(env);
        EventRegistrationToken token{};
        ok = SUCCEEDED(core->add_WebResourceRequested(server, &token));
        server->Release();
      }
      env->Release();
    }
    core2->Release();
  }
  return ok;
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
  if (fullscreen) wp = savedPlacement;  // en plein écran : on garde la fenêtre d'avant
  if (fullscreen || GetWindowPlacement(win, &wp)) {
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

static void setFullscreen(bool on);

static void setCompact(bool on) {
  if (on == compact) return;
  if (on && fullscreen) setFullscreen(false);
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

// Plein écran : fenêtre sans bordure ni barre de titre, sur tout l'écran (barre des tâches comprise)
static void setFullscreen(bool on) {
  if (on == fullscreen) return;
  if (on) {
    if (compact) setCompact(false);
    savedPlacement.length = sizeof(savedPlacement);
    GetWindowPlacement(win, &savedPlacement);
    savedStyle = GetWindowLongPtrW(win, GWL_STYLE);
    savedExStyle = GetWindowLongPtrW(win, GWL_EXSTYLE);
    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoW(MonitorFromWindow(win, MONITOR_DEFAULTTONEAREST), &mi);
    fullscreen = true;
    SetWindowLongPtrW(win, GWL_STYLE, savedStyle & ~(WS_CAPTION | WS_THICKFRAME));
    SetWindowLongPtrW(win, GWL_EXSTYLE,
                      savedExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
    SetWindowPos(win, onTop ? HWND_TOPMOST : HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                 mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                 SWP_NOACTIVATE | SWP_FRAMECHANGED);
  } else {
    fullscreen = false;
    SetWindowLongPtrW(win, GWL_STYLE, savedStyle);
    SetWindowLongPtrW(win, GWL_EXSTYLE, savedExStyle);
    SetWindowPos(win, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    SetWindowPlacement(win, &savedPlacement);
  }
}

static void setOnTop(bool on) {
  onTop = on;
  SetWindowPos(win, on ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

// ---- Zone de notification : icône près de l'horloge, et notification de Windows -------------------
// Quand KeoLive n'est pas au premier plan, un incident est annoncé par une notification
// de Windows ; un clic dessus ramène la fenêtre et ouvre la fiche de la ligne.
static const UINT WM_TRAY = WM_APP + 7;
static NOTIFYICONDATAW tray{};
static bool trayAdded = false;
static webview::webview* view = nullptr;

static bool addTray(HICON icon) {
  tray.cbSize = sizeof(tray);
  tray.hWnd = win;
  tray.uID = 1;
  tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  tray.uCallbackMessage = WM_TRAY;
  tray.hIcon = icon;
  wcscpy(tray.szTip, L"KeoLive");
  trayAdded = Shell_NotifyIconW(NIM_ADD, &tray) != FALSE;
  return trayAdded;
}

static void removeTray() {
  if (trayAdded) Shell_NotifyIconW(NIM_DELETE, &tray);
  trayAdded = false;
}

static bool notify(const std::wstring& title, const std::wstring& text) {
  if (!trayAdded) return false;
  tray.uFlags = NIF_INFO;
  wcsncpy(tray.szInfoTitle, title.c_str(), ARRAYSIZE(tray.szInfoTitle) - 1);
  tray.szInfoTitle[ARRAYSIZE(tray.szInfoTitle) - 1] = 0;
  wcsncpy(tray.szInfo, text.empty() ? L" " : text.c_str(), ARRAYSIZE(tray.szInfo) - 1);
  tray.szInfo[ARRAYSIZE(tray.szInfo) - 1] = 0;
  tray.dwInfoFlags = NIIF_WARNING | NIIF_NOSOUND;  // le son vient de l'application (réglage « Son des alertes »)
  const bool ok = Shell_NotifyIconW(NIM_MODIFY, &tray) != FALSE;
  FLASHWINFO fw{sizeof(fw), win, FLASHW_TRAY | FLASHW_TIMERNOFG, 3, 0};  // le bouton de la barre des tâches clignote
  FlashWindowEx(&fw);
  return ok;
}

static void showMain() {
  if (IsIconic(win)) ShowWindow(win, SW_RESTORE);
  SetForegroundWindow(win);
}

static LRESULT CALLBACK subclass(HWND h, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
  if (msg == WM_CLOSE || msg == WM_EXITSIZEMOVE) saveState();
  if (msg == WM_DESTROY) removeTray();
  if (msg == WM_TRAY) {
    const UINT ev = LOWORD(lp);
    if (ev == NIN_BALLOONUSERCLICK) {  // clic sur la notification : la fiche de la ligne
      showMain();
      if (view) view->eval("window.keoNotifyClick&&window.keoNotifyClick()");
    } else if (ev == WM_LBUTTONUP || ev == WM_LBUTTONDBLCLK) showMain();
    return 0;
  }
  // Fenêtre réduite : la page passe en arrière-plan (plus d'appels à l'API)
  if (msg == WM_SIZE && controller) controller->put_IsVisible(wp == SIZE_MINIMIZED ? FALSE : TRUE);
  if (msg == WM_GETMINMAXINFO) {  // taille minimale : celle du mode compact
    auto* mm = (MINMAXINFO*)lp;
    mm->ptMinTrackSize.x = dpiScale(360);
    mm->ptMinTrackSize.y = dpiScale(260);
  }
  return DefSubclassProc(h, msg, wp, lp);
}

// Barre de titre assortie au thème de l'application (couleurs : Windows 11 ; ignorées ailleurs)
static void titleBar(bool light) {
  BOOL dark = light ? FALSE : TRUE;
  DwmSetWindowAttribute(win, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
  COLORREF caption = light ? RGB(0xff, 0xff, 0xff) : RGB(0x0a, 0x0f, 0x19);
  COLORREF text = light ? RGB(0x0b, 0x12, 0x20) : RGB(0xf3, 0xf5, 0xf9);
  COLORREF border = light ? RGB(0xd8, 0xde, 0xe8) : RGB(0x1a, 0x23, 0x32);
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

// Chaîne JSON ("…", avec ses échappements) -> texte
static std::wstring jsonText(const std::string& j) {
  std::string o;
  if (j.size() < 2 || j.front() != '"') return std::wstring();
  for (size_t i = 1; i + 1 < j.size(); i++) {
    char c = j[i];
    if (c != '\\') { o += c; continue; }
    if (++i + 1 >= j.size()) break;
    switch (j[i]) {
      case 'n': o += '\n'; break;
      case 't': o += ' '; break;
      case 'r': break;
      case 'u': {
        if (i + 4 < j.size()) {
          unsigned cp = (unsigned)strtoul(j.substr(i + 1, 4).c_str(), nullptr, 16);
          i += 4;
          if (cp < 0x80) o += (char)cp;
          else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3F)); }
          else { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
        }
        break;
      }
      default: o += j[i]; break;  // \" \\ \/
    }
  }
  return widen(o);
}

static std::string nativeCall(const std::string& req) {
  std::string cmd, arg;
  parseRequest(req, cmd, arg);
  if (cmd == "ontop") setOnTop(arg == "true");
  else if (cmd == "compact") setCompact(arg == "true");
  else if (cmd == "fullscreen") {
    setFullscreen(arg == "true");
    return fullscreen ? "true" : "false";
  }
  else if (cmd == "theme") titleBar(arg == "\"light\"");
  else if (cmd == "notify") {  // "titre\ntexte"
    const std::wstring all = jsonText(arg);
    const size_t nl = all.find(L'\n');
    notify(all.substr(0, nl), nl == std::wstring::npos ? std::wstring() : all.substr(nl + 1));
  }
  else if (cmd == "open" && arg.size() > 2 && arg.front() == '"' && arg.back() == '"') {
    std::string url = arg.substr(1, arg.size() - 2);  // adresse web seulement
    if (url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0)
      ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  }
  else if (cmd == "version") return std::string("\"") + LAUNCHER_VERSION + "\"";
  return "true";
}

// ---- Démarrage -----------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
  // Une seule fenêtre : un second lancement ramène la première au premier plan
  HANDLE single = CreateMutexW(nullptr, TRUE, L"Local\\KeoLivePourWindows");
  if (single && GetLastError() == ERROR_ALREADY_EXISTS) {
    HWND other = FindWindowW(L"webview", L"KeoLive");
    if (other) {
      ShowWindow(other, SW_RESTORE);
      SetForegroundWindow(other);
    }
    return 0;
  }

  pagePath = exeDir() + L"\\KeoLive.html";
  if (!readPage()) {
    const std::wstring msg = L"Le fichier KeoLive.html est introuvable.\n\nIl doit se trouver dans le même dossier que "
                             L"KeoLive.exe :\n" + exeDir() + L"\n\nTéléchargez KeoLive.zip et utilisez « Extraire tout » "
                             L"avant de lancer KeoLive.exe.";
    MessageBoxW(nullptr, msg.c_str(), L"KeoLive", MB_ICONERROR);
    return 1;
  }
  dataDir = localAppData() + L"\\KeoLive";
  CreateDirectoryW(dataDir.c_str(), nullptr);
  // Données de WebView2 (réglages de la page, cache) : à côté, plutôt qu'à côté de l'exécutable
  SetEnvironmentVariableW(L"WEBVIEW2_USER_DATA_FOLDER", (dataDir + L"\\WebView2").c_str());
  // Le carillon des alertes doit sonner même si personne n'a encore cliqué dans la fenêtre
  SetEnvironmentVariableW(L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", L"--autoplay-policy=no-user-gesture-required");
  loadState();

  try {
    webview::webview w(false, nullptr);
    win = (HWND)w.window().value();
    SetWindowTextW(win, L"KeoLive");
    HICON big = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    HICON small = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    if (big) SendMessageW(win, WM_SETICON, ICON_BIG, (LPARAM)big);
    if (small) SendMessageW(win, WM_SETICON, ICON_SMALL, (LPARAM)small);
    titleBar(false);
    SetWindowSubclass(win, subclass, 1, 0);
    addTray(small ? small : big);  // icône près de l'horloge : notifications d'incident
    view = &w;

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

    // Page servie depuis la mémoire, sous une adresse fixe
    controller = (ICoreWebView2Controller*)w.browser_controller().value();
    ICoreWebView2* core = nullptr;
    bool served = false;
    if (controller && SUCCEEDED(controller->get_CoreWebView2(&core)) && core) {
      served = servePage(core);
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

    if (!served) throw std::runtime_error("WebView2 trop ancien");

    w.bind("metroNative", nativeCall);
    w.navigate("https://app.metro.example/index.html");
    w.run();
    view = nullptr;
    removeTray();
  } catch (const std::exception&) {
    if (MessageBoxW(nullptr,
                    L"Le moteur WebView2 de Microsoft est introuvable sur cet ordinateur.\n\n"
                    L"Il est inclus dans Windows 11 ; sinon, installez « Microsoft Edge WebView2 Runtime » "
                    L"(gratuit). Ouvrir la page de téléchargement ?",
                    L"KeoLive", MB_ICONWARNING | MB_YESNO) == IDYES)
      ShellExecuteW(nullptr, L"open", L"https://developer.microsoft.com/microsoft-edge/webview2/", nullptr, nullptr,
                    SW_SHOWNORMAL);
    return 1;
  }
  return 0;
}
