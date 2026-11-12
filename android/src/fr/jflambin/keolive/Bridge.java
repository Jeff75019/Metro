package fr.jflambin.keolive;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.PowerManager;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.provider.Settings;
import android.webkit.JavascriptInterface;

import org.json.JSONException;
import org.json.JSONObject;

/** Fonctions offertes à la page sous window.KeoAndroid (appelées hors du fil d'interface). */
public final class Bridge {
  private final MainActivity a;

  Bridge(MainActivity a) { this.a = a; }

  /** Ce que la page a besoin de savoir au démarrage */
  @JavascriptInterface public String getConfig() {
    JSONObject o = new JSONObject();
    try {
      String v = "?";
      try { v = a.getPackageManager().getPackageInfo(a.getPackageName(), 0).versionName; } catch (PackageManager.NameNotFoundException ignored) { }
      o.put("version", v);
      o.put("sdk", Build.VERSION.SDK_INT);
      o.put("systemDark", (a.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES);
      o.put("notifications", a.notificationsState());
      PowerManager pm = a.getSystemService(PowerManager.class);
      o.put("batteryExempt", pm != null && pm.isIgnoringBatteryOptimizations(a.getPackageName()));
    } catch (JSONException ignored) { }
    return o.toString();
  }

  /** Réglages utiles en arrière-plan : {apiKey, alerts, followed, pinned} */
  @JavascriptInterface public void setState(String json) {
    try {
      new Prefs(a).setState(new JSONObject(json));
      AlertJob.sync(a);
    } catch (JSONException ignored) { }
  }

  /** Signe de vie de la page (toutes les minutes) : tant qu'il arrive, la tâche de fond laisse la page surveiller */
  @JavascriptInterface public void alive() { new Prefs(a).touch(); }

  /** Incidents que la page a déjà montrés : {id: {level, lines, title}} */
  @JavascriptInterface public void seen(String json) {
    try { new Prefs(a).setKnown(new JSONObject(json)); } catch (JSONException ignored) { }
  }

  /** Couleur derrière les barres système et teinte de leurs icônes, assorties au thème de la page */
  @JavascriptInterface public void setBars(boolean dark, String hex) {
    int color;
    try { color = Color.parseColor(hex); } catch (IllegalArgumentException e) { color = dark ? 0xFF0A0F19 : 0xFFFFFFFF; }
    a.setBars(dark, color);
  }

  @JavascriptInterface public void openUrl(String url) {
    if (url != null && (url.startsWith("https://") || url.startsWith("http://"))) a.openExternal(Uri.parse(url));
  }

  @JavascriptInterface public void vibrate(int ms) {
    Vibrator v = a.getSystemService(Vibrator.class);
    if (v != null && v.hasVibrator()) v.vibrate(VibrationEffect.createOneShot(Math.max(10, Math.min(1000, ms)), VibrationEffect.DEFAULT_AMPLITUDE));
  }

  @JavascriptInterface public void requestNotifications() { a.runOnUiThread(() -> a.askNotifications(true)); }

  @JavascriptInterface public void openNotificationSettings() {
    Intent i = new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS).putExtra(Settings.EXTRA_APP_PACKAGE, a.getPackageName());
    a.runOnUiThread(() -> { try { a.startActivity(i); } catch (RuntimeException ignored) { } });
  }

  @JavascriptInterface public void testNotification() { Notifier.test(a); }

  /** Demande à Android de ne pas brider KeoLive en arrière-plan (alertes plus régulières) */
  @JavascriptInterface public void requestBackground() {
    a.runOnUiThread(() -> {
      try {
        a.startActivity(new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS, Uri.parse("package:" + a.getPackageName())));
      } catch (RuntimeException e) {
        try { a.startActivity(new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS)); } catch (RuntimeException ignored) { }
      }
    });
  }
}
