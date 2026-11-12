package fr.jflambin.keolive;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;

import java.util.List;
import java.util.Map;

/** Notifications d'incident : un canal « Incidents » (sonore, prioritaire) et un canal
 *  « Trafic rétabli » (discret). Un toucher ouvre KeoLive sur la fiche de la ligne. */
final class Notifier {
  static final String CH_INCIDENTS = "incidents";
  static final String CH_END = "retabli";

  private Notifier() {}

  static void ensureChannels(Context c) {
    NotificationManager nm = c.getSystemService(NotificationManager.class);
    NotificationChannel inc = new NotificationChannel(CH_INCIDENTS, c.getString(R.string.channel_incidents), NotificationManager.IMPORTANCE_HIGH);
    inc.setDescription(c.getString(R.string.channel_incidents_desc));
    inc.enableVibration(true);
    inc.setShowBadge(true);
    NotificationChannel end = new NotificationChannel(CH_END, c.getString(R.string.channel_end), NotificationManager.IMPORTANCE_DEFAULT);
    end.setDescription(c.getString(R.string.channel_end_desc));
    nm.createNotificationChannel(inc);
    nm.createNotificationChannel(end);
  }

  static String lineLabel(Map<String, Incidents.Line> lines, String id) {
    Incidents.Line li = lines.get(id);
    if (li == null) return "Ligne";
    String mode = li.m.equals("metro") ? "Métro" : li.m.equals("rer") ? "RER" : li.m.equals("tram") ? "Tram" : li.m.equals("train") ? "Train" : "Bus";
    return mode + " " + li.n;
  }

  /** kind : new (nouvel incident), worse (aggravation), end (trafic rétabli) */
  static void show(Context c, Incidents.Incident g, String kind, Map<String, Incidents.Line> lines) {
    boolean end = kind.equals("end");
    StringBuilder who = new StringBuilder();
    List<String> ids = g.lines.subList(0, Math.min(3, g.lines.size()));
    for (String id : ids) who.append(who.length() > 0 ? ", " : "").append(lineLabel(lines, id));
    String state = end ? "trafic rétabli" : g.level == 3 ? "trafic interrompu" : "trafic perturbé";
    String title = who + " : " + (kind.equals("worse") ? "aggravation, " : "") + state;
    String text = !g.title.isEmpty() ? g.title : !g.shortMsg.isEmpty() ? g.shortMsg : "Perturbation";
    String big = end ? text : (g.msg.isEmpty() ? text : (g.title.isEmpty() ? "" : g.title + "\n\n") + g.msg);

    Intent open = new Intent(c, MainActivity.class)
        .putExtra(MainActivity.EXTRA_LINE, g.lines.isEmpty() ? "" : g.lines.get(0))
        .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
    int id = g.id.hashCode();
    PendingIntent pi = PendingIntent.getActivity(c, id, open, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

    int color = 0xFFFF8C1A;
    try {
      Incidents.Line li = lines.get(g.lines.isEmpty() ? "" : g.lines.get(0));
      if (li != null) color = Color.parseColor(li.c);
    } catch (IllegalArgumentException ignored) { /* couleur illisible : orange */ }

    Notification n = new Notification.Builder(c, end ? CH_END : CH_INCIDENTS)
        .setSmallIcon(R.drawable.ic_notif)
        .setColor(color)
        .setContentTitle(title)
        .setContentText(text)
        .setStyle(new Notification.BigTextStyle().bigText(big))
        .setContentIntent(pi)
        .setAutoCancel(true)
        .setCategory(Notification.CATEGORY_EVENT)
        .setShowWhen(true)
        .build();
    c.getSystemService(NotificationManager.class).notify(id, n);
  }

  /** Notification d'essai (réglages), pour vérifier qu'Android les laisse passer */
  static void test(Context c) {
    Intent open = new Intent(c, MainActivity.class).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    PendingIntent pi = PendingIntent.getActivity(c, 7, open, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
    Notification n = new Notification.Builder(c, CH_INCIDENTS)
        .setSmallIcon(R.drawable.ic_notif)
        .setColor(0xFFFFBE00)
        .setContentTitle("Métro 1 : trafic interrompu")
        .setContentText("Essai de notification de KeoLive")
        .setStyle(new Notification.BigTextStyle().bigText("Essai de notification de KeoLive. Les vraies alertes arrivent ainsi, même application fermée."))
        .setContentIntent(pi)
        .setAutoCancel(true)
        .build();
    c.getSystemService(NotificationManager.class).notify(7, n);
  }
}
