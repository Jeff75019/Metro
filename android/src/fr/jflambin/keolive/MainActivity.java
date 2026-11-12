package fr.jflambin.keolive;

import android.Manifest;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.FrameLayout;

import org.json.JSONObject;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

/** KeoLive pour Android : la page de l'application (assets/www) dans une WebView, servie
 *  sous https://app.keolive.example/ (origine fixe : réglages et épingles conservés).
 *  Le reste (alertes en arrière-plan, notifications) est dans AlertJob et Notifier. */
public class MainActivity extends Activity {
  static final String HOST = "app.keolive.example";
  static final String EXTRA_LINE = "line";
  private static final int REQ_NOTIF = 1;

  private FrameLayout root;
  private WebView web;
  private boolean pageReady = false;
  private String pendingLine;

  @Override protected void onCreate(Bundle saved) {
    super.onCreate(saved);
    Notifier.ensureChannels(this);
    root = new FrameLayout(this);
    root.setBackgroundColor(0xFF0A0F19);
    web = new WebView(this);
    root.addView(web, new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
    setContentView(root);
    setupInsets();

    WebSettings s = web.getSettings();
    s.setJavaScriptEnabled(true);
    s.setDomStorageEnabled(true);
    s.setMediaPlaybackRequiresUserGesture(false);   // le carillon des alertes
    s.setSupportZoom(false);
    s.setBuiltInZoomControls(false);
    s.setDisplayZoomControls(false);
    s.setTextZoom(100);
    s.setAllowFileAccess(false);
    s.setAllowContentAccess(false);
    s.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
    web.setBackgroundColor(0xFF05070C);
    web.setOverScrollMode(View.OVER_SCROLL_NEVER);
    web.setWebViewClient(new WebViewClient() {
      @Override public WebResourceResponse shouldInterceptRequest(WebView v, WebResourceRequest r) {
        Uri u = r.getUrl();
        return HOST.equals(u.getHost()) ? asset(u.getPath()) : null;
      }
      @Override public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest r) {
        Uri u = r.getUrl();
        if (HOST.equals(u.getHost())) return false;
        openExternal(u);
        return true;
      }
      @Override public void onPageFinished(WebView v, String url) {
        pageReady = true;
        deliverPendingLine();
      }
    });
    web.addJavascriptInterface(new Bridge(this), "KeoAndroid");
    web.loadUrl("https://" + HOST + "/index.html");
    handleIntent(getIntent());
    askNotifications(false);
  }

  // ---- La page, depuis les ressources de l'application ------------------------------------------
  private WebResourceResponse asset(String path) {
    if (path == null || path.isEmpty() || path.equals("/")) path = "/index.html";
    String mime = mimeOf(path);
    try {
      InputStream in = getAssets().open("www" + path);
      Map<String, String> h = new HashMap<>();
      h.put("Cache-Control", "no-cache");
      boolean text = mime.startsWith("text/") || mime.contains("javascript") || mime.contains("json");
      return new WebResourceResponse(mime, text ? "utf-8" : null, 200, "OK", h, in);
    } catch (IOException e) {
      return new WebResourceResponse("text/plain", "utf-8", 404, "Not Found", new HashMap<>(), new ByteArrayInputStream(new byte[0]));
    }
  }

  private static String mimeOf(String p) {
    if (p.endsWith(".html")) return "text/html";
    if (p.endsWith(".css")) return "text/css";
    if (p.endsWith(".js")) return "application/javascript";
    if (p.endsWith(".json")) return "application/json";
    if (p.endsWith(".woff2")) return "font/woff2";
    if (p.endsWith(".png")) return "image/png";
    if (p.endsWith(".svg")) return "image/svg+xml";
    return "application/octet-stream";
  }

  // ---- Barres système : la page occupe tout l'écran, l'activité garde la place des barres -------------
  private void setupInsets() {
    Window w = getWindow();
    if (Build.VERSION.SDK_INT >= 30) w.setDecorFitsSystemWindows(false);
    else w.getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
    w.setStatusBarColor(Color.TRANSPARENT);
    w.setNavigationBarColor(Color.TRANSPARENT);
    if (Build.VERSION.SDK_INT >= 29) {
      w.setStatusBarContrastEnforced(false);
      w.setNavigationBarContrastEnforced(false);
    }
    root.setOnApplyWindowInsetsListener((v, insets) -> {
      int l, t, r, b;
      if (Build.VERSION.SDK_INT >= 30) {
        android.graphics.Insets i = insets.getInsets(WindowInsets.Type.systemBars() | WindowInsets.Type.displayCutout() | WindowInsets.Type.ime());
        l = i.left; t = i.top; r = i.right; b = i.bottom;
      } else {
        l = insets.getSystemWindowInsetLeft(); t = insets.getSystemWindowInsetTop();
        r = insets.getSystemWindowInsetRight(); b = insets.getSystemWindowInsetBottom();
      }
      v.setPadding(l, t, r, b);
      return Build.VERSION.SDK_INT >= 30 ? WindowInsets.CONSUMED : insets.consumeSystemWindowInsets();
    });
  }

  /** Couleur derrière les barres (celle de l'en-tête de la page) et teinte de leurs icônes */
  void setBars(boolean dark, int color) {
    runOnUiThread(() -> {
      root.setBackgroundColor(color);
      int light = WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS | WindowInsetsController.APPEARANCE_LIGHT_NAVIGATION_BARS;
      if (Build.VERSION.SDK_INT >= 30) {
        WindowInsetsController c = getWindow().getInsetsController();
        if (c != null) c.setSystemBarsAppearance(dark ? 0 : light, light);
      } else {
        View d = getWindow().getDecorView();
        int f = d.getSystemUiVisibility();
        int flags = View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR | View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR;
        d.setSystemUiVisibility(dark ? f & ~flags : f | flags);
      }
    });
  }

  // ---- Notifications : permission (Android 13 et plus) -------------------------------------------------
  String notificationsState() {
    if (Build.VERSION.SDK_INT < 33) return "granted";
    return checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED ? "granted" : "denied";
  }

  void askNotifications(boolean force) {
    if (Build.VERSION.SDK_INT < 33 || notificationsState().equals("granted")) return;
    Prefs p = new Prefs(this);
    if (!force && p.askedNotifications()) return;
    p.setAskedNotifications();
    requestPermissions(new String[] {Manifest.permission.POST_NOTIFICATIONS}, REQ_NOTIF);
  }

  @Override public void onRequestPermissionsResult(int code, String[] perms, int[] results) {
    super.onRequestPermissionsResult(code, perms, results);
    if (code == REQ_NOTIF) js("window.keoNotifications&&keoNotifications(" + JSONObject.quote(notificationsState()) + ")");
  }

  // ---- Ouverture depuis une notification : la fiche de la ligne --------------------------------------
  @Override protected void onNewIntent(Intent i) {
    super.onNewIntent(i);
    setIntent(i);
    handleIntent(i);
  }

  private void handleIntent(Intent i) {
    String line = i == null ? null : i.getStringExtra(EXTRA_LINE);
    if (line != null && !line.isEmpty()) {
      pendingLine = line;
      deliverPendingLine();
    }
  }

  private void deliverPendingLine() {
    if (!pageReady || pendingLine == null) return;
    String l = pendingLine;
    pendingLine = null;
    js("window.keoOpenLine&&keoOpenLine(" + JSONObject.quote(l) + ")");
  }

  void openExternal(Uri u) {
    try { startActivity(new Intent(Intent.ACTION_VIEW, u)); } catch (ActivityNotFoundException ignored) { }
  }

  private void js(String code) { runOnUiThread(() -> web.evaluateJavascript(code, null)); }

  // ---- Cycle de vie ------------------------------------------------------------------------------------
  @Override protected void onResume() {
    super.onResume();
    new Prefs(this).setForeground(true);
    web.onResume();
    js("window.keoResume&&keoResume()");
  }

  @Override protected void onPause() {
    new Prefs(this).setForeground(false);
    web.onPause();
    super.onPause();
  }

  @Override public void onConfigurationChanged(Configuration c) {
    super.onConfigurationChanged(c);
    boolean dark = (c.uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
    js("window.keoSystemTheme&&keoSystemTheme(" + dark + ")");
  }

  /** Retour : la page ferme d'abord ce qu'elle a d'ouvert (feuille, alerte, onglet) ; sinon l'application passe en arrière-plan */
  @Override public void onBackPressed() {
    web.evaluateJavascript("window.keoBack?keoBack():false", v -> { if (!"true".equals(v)) moveTaskToBack(true); });
  }

  @Override protected void onDestroy() {
    root.removeView(web);
    web.destroy();
    super.onDestroy();
  }
}
