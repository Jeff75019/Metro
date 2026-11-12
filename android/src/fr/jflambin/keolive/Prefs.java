package fr.jflambin.keolive;

import android.content.Context;
import android.content.SharedPreferences;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashSet;
import java.util.Set;

/** Ce que la partie native doit connaître de la page : clé PRIM, réglage des alertes, lignes
 *  surveillées, incidents déjà vus. Écrit par la page (pont KeoAndroid), lu par le travail
 *  d'arrière-plan. */
final class Prefs {
  private final SharedPreferences p;

  Prefs(Context c) { p = c.getSharedPreferences("keolive", Context.MODE_PRIVATE); }

  String apiKey() { return p.getString("apiKey", ""); }
  String alerts() { return p.getString("alerts", "all"); }   // all | mine | off
  /** La page est à l'écran et donne signe de vie (toutes les minutes) ; un drapeau resté levé
   *  après un arrêt brutal ne compte plus au bout de cinq minutes. */
  boolean foreground() {
    return p.getBoolean("foreground", false) && System.currentTimeMillis() - p.getLong("alive", 0) < 5 * 60 * 1000L;
  }
  void touch() { p.edit().putLong("alive", System.currentTimeMillis()).apply(); }
  boolean askedNotifications() { return p.getBoolean("askedNotif", false); }

  void setForeground(boolean on) { p.edit().putBoolean("foreground", on).putLong("alive", on ? System.currentTimeMillis() : 0).apply(); }
  void setAskedNotifications() { p.edit().putBoolean("askedNotif", true).apply(); }

  /** État envoyé par la page : {apiKey, alerts, followed:[…], pinned:[…]} */
  void setState(JSONObject o) {
    SharedPreferences.Editor e = p.edit();
    e.putString("apiKey", o.optString("apiKey", ""));
    e.putString("alerts", o.optString("alerts", "all"));
    e.putString("followed", String.valueOf(o.optJSONArray("followed") == null ? new JSONArray() : o.optJSONArray("followed")));
    e.putString("pinned", String.valueOf(o.optJSONArray("pinned") == null ? new JSONArray() : o.optJSONArray("pinned")));
    e.apply();
  }

  /** Lignes surveillées en mode « Mes lignes » : lignes suivies et lignes des arrêts épinglés */
  Set<String> myLines() {
    Set<String> s = new HashSet<>();
    for (String k : new String[] {"followed", "pinned"}) {
      try {
        JSONArray a = new JSONArray(p.getString(k, "[]"));
        for (int i = 0; i < a.length(); i++) s.add(a.getString(i));
      } catch (JSONException ignored) { /* valeur absente ou abîmée : ignorée */ }
    }
    return s;
  }

  /** Incidents connus : {id: {level, lines:[…], title}} ; null tant qu'aucune lecture n'a eu lieu */
  JSONObject known() {
    String s = p.getString("known", null);
    if (s == null) return null;
    try { return new JSONObject(s); } catch (JSONException e) { return null; }
  }
  void setKnown(JSONObject o) { p.edit().putString("known", o == null ? null : o.toString()).apply(); }
  void clearKnown() { p.edit().remove("known").apply(); }
}
