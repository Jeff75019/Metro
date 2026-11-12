package fr.jflambin.keolive;

import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.ComponentName;
import android.content.Context;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.InputStream;
import java.util.Map;
import java.util.Set;

/** Alertes quand KeoLive n'est pas à l'écran : toutes les 15 minutes environ (le minimum
 *  d'Android, qui peut espacer davantage quand le téléphone dort), lecture de l'info trafic
 *  et notification des incidents nouveaux, aggravés ou terminés. Quand la page est au
 *  premier plan, c'est elle qui surveille et le travail ne fait rien. */
public class AlertJob extends JobService {
  private static final String TAG = "KeoLive";
  private static final int JOB_ID = 1;
  private static final long PERIOD_MS = 15 * 60 * 1000L;

  /** Programme ou retire le travail selon les réglages (clé présente, alertes actives) */
  static void sync(Context c) {
    Prefs p = new Prefs(c);
    JobScheduler js = c.getSystemService(JobScheduler.class);
    boolean want = !p.apiKey().isEmpty() && !"off".equals(p.alerts());
    if (!want) { js.cancel(JOB_ID); return; }
    if (js.getPendingJob(JOB_ID) != null) return;   // déjà en place : on ne remet pas le compteur à zéro
    JobInfo info = new JobInfo.Builder(JOB_ID, new ComponentName(c, AlertJob.class))
        .setPeriodic(PERIOD_MS)
        .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
        .setPersisted(true)
        .build();
    js.schedule(info);
  }

  @Override public boolean onStartJob(JobParameters params) {
    new Thread(() -> {
      try { check(getApplicationContext()); } catch (Throwable t) { Log.w(TAG, "alertes : " + t); }
      jobFinished(params, false);
    }, "keolive-alertes").start();
    return true;
  }

  @Override public boolean onStopJob(JobParameters params) { return false; }

  /** Une lecture : compare avec les incidents connus, notifie, mémorise */
  static void check(Context c) throws Exception {
    Prefs p = new Prefs(c);
    if (p.foreground()) return;   // la page s'en charge elle-même
    Notifier.ensureChannels(c);   // au cas où l'application n'aurait pas été ouverte depuis une mise à jour
    String key = p.apiKey();
    if (key.isEmpty() || "off".equals(p.alerts())) return;
    Map<String, Incidents.Line> lines;
    try (InputStream in = c.getAssets().open("lines.json")) { lines = Incidents.loadLines(in); }
    Set<String> only = "mine".equals(p.alerts()) ? p.myLines() : null;
    Map<String, Incidents.Incident> cur = Incidents.parse(Incidents.fetch(key), lines, System.currentTimeMillis(), only);

    JSONObject known = p.known();
    if (known == null) {   // première lecture : rien à annoncer (comme la page au lancement)
      p.setKnown(toJson(cur));
      return;
    }
    for (Incidents.Incident g : cur.values()) {
      JSONObject old = known.optJSONObject(g.id);
      if (old == null) Notifier.show(c, g, "new", lines);
      else if (g.level > old.optInt("level")) Notifier.show(c, g, "worse", lines);
    }
    for (java.util.Iterator<String> it = known.keys(); it.hasNext();) {
      String id = it.next();
      if (cur.containsKey(id)) continue;
      Incidents.Incident g = fromJson(id, known.getJSONObject(id));
      if (!g.lines.isEmpty()) Notifier.show(c, g, "end", lines);
    }
    p.setKnown(toJson(cur));
  }

  static JSONObject toJson(Map<String, Incidents.Incident> cur) throws JSONException {
    JSONObject o = new JSONObject();
    for (Incidents.Incident g : cur.values()) {
      JSONObject k = new JSONObject();
      k.put("level", g.level);
      k.put("lines", new JSONArray(g.lines));
      k.put("title", g.title.isEmpty() ? g.shortMsg : g.title);
      o.put(g.id, k);
    }
    return o;
  }

  static Incidents.Incident fromJson(String id, JSONObject k) {
    Incidents.Incident g = new Incidents.Incident();
    g.id = id;
    g.level = k.optInt("level", 2);
    g.title = k.optString("title", "");
    JSONArray a = k.optJSONArray("lines");
    if (a != null) for (int i = 0; i < a.length(); i++) g.lines.add(a.optString(i));
    return g;
  }
}
