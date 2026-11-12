package fr.jflambin.keolive;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Info trafic PRIM (disruptions_bulk v2) : lecture et décodage des incidents en cours,
 *  même logique que parseTraffic() et incidents() de la page (windows/app/app.js).
 *  Java pur (pas d'API Android) : testable sur un ordinateur avec tools/IncidentsTest.java. */
public final class Incidents {
  public static final String URL = "https://prim.iledefrance-mobilites.fr/marketplace/disruptions_bulk/disruptions/v2";
  private static final ZoneId PARIS = ZoneId.of("Europe/Paris");
  private static final DateTimeFormatter COMPACT = DateTimeFormatter.ofPattern("yyyyMMdd'T'HHmmss");
  private static final Pattern LINE_ID = Pattern.compile("\\b(C\\d{5}|M1[67])\\b");
  private static final Pattern TITLE_PREFIX =
      Pattern.compile("^(M[ée]tro|RER|Tram(way)?|Train|Ligne|Transilien)?\\s*[\\p{L}\\p{N}_-]{0,4}\\s*:\\s*", Pattern.CASE_INSENSITIVE);

  /** Une ligne du réseau (lines.json, généré depuis windows/app/data.js) */
  public static final class Line {
    public final String id, n, m, c, t;
    public final int rank;
    Line(String id, String n, String m, String c, String t, int rank) { this.id = id; this.n = n; this.m = m; this.c = c; this.t = t; this.rank = rank; }
  }

  /** Un incident en cours (trafic perturbé ou interrompu), avec les lignes qu'il touche */
  public static final class Incident {
    public String id;
    public final List<String> lines = new ArrayList<>();
    public int level;          // 2 : perturbé, 3 : interrompu
    public boolean works;
    public String title = "", shortMsg = "", msg = "";
    public long begin, end;
  }

  private Incidents() {}

  public static Map<String, Line> loadLines(InputStream in) throws IOException, JSONException {
    JSONArray a = new JSONArray(read(in));
    Map<String, Line> out = new LinkedHashMap<>();
    for (int i = 0; i < a.length(); i++) {
      JSONObject o = a.getJSONObject(i);
      out.put(o.getString("id"), new Line(o.getString("id"), o.getString("n"), o.getString("m"), o.getString("c"), o.getString("t"), i));
    }
    return out;
  }

  /** Lecture de l'info trafic ; IOException « PRIM 401 » etc. si le serveur refuse */
  public static String fetch(String apiKey) throws IOException {
    HttpURLConnection c = (HttpURLConnection) new URL(URL).openConnection();
    c.setConnectTimeout(15000);
    c.setReadTimeout(40000);
    c.setRequestProperty("apikey", apiKey);
    c.setRequestProperty("Accept", "application/json");
    try {
      int code = c.getResponseCode();
      if (code != 200) throw new IOException("PRIM " + code);
      try (InputStream in = c.getInputStream()) { return read(in); }
    } finally {
      c.disconnect();
    }
  }

  /** Incidents en cours à l'instant now, par identifiant de perturbation.
   *  only : lignes surveillées (null : toutes). Seules les lignes ferrées connues comptent. */
  public static Map<String, Incident> parse(String body, Map<String, Line> lines, long now, Set<String> only) throws JSONException {
    JSONObject json = new JSONObject(body);
    Map<String, JSONObject> dis = new HashMap<>();
    JSONArray da = json.optJSONArray("disruptions");
    if (da != null) for (int i = 0; i < da.length(); i++) {
      JSONObject d = da.getJSONObject(i);
      dis.put(d.optString("id"), d);
    }
    Map<String, Incident> out = new LinkedHashMap<>();
    JSONArray la = json.optJSONArray("lines");
    if (la != null) for (int i = 0; i < la.length(); i++) {
      JSONObject L = la.getJSONObject(i);
      String id = lineIdOf(L.optString("id"));
      Line li = lines.get(id);
      if (li == null || !(li.m.equals("metro") || li.m.equals("rer") || li.m.equals("tram") || li.m.equals("train"))) continue;
      if (only != null && !only.contains(id)) continue;
      Set<String> ids = new HashSet<>();
      JSONArray objs = L.optJSONArray("impactedObjects");
      if (objs != null) for (int k = 0; k < objs.length(); k++) {
        JSONArray dids = objs.getJSONObject(k).optJSONArray("disruptionIds");
        if (dids != null) for (int q = 0; q < dids.length(); q++) ids.add(dids.getString(q));
      }
      for (String did : ids) {
        JSONObject d = dis.get(did);
        if (d == null) continue;
        String sev = d.optString("severity");
        int level = sev.equals("BLOQUANTE") ? 3 : sev.equals("PERTURBEE") ? 2 : 1;
        if (level < 2) continue;
        long begin = 0, end = 0;
        boolean active = false;
        JSONArray periods = d.optJSONArray("applicationPeriods");
        if (periods != null) for (int k = 0; k < periods.length(); k++) {
          JSONObject p = periods.getJSONObject(k);
          long b = parseCompact(p.optString("begin")), e = parseCompact(p.optString("end"));
          if (b > 0 && e > 0 && b <= now && now <= e) { active = true; begin = b; end = e; break; }
        }
        if (!active) continue;
        Incident g = out.get(did);
        if (g == null) {
          g = new Incident();
          g.id = did;
          g.level = level;
          g.works = "TRAVAUX".equals(d.optString("cause"));
          g.title = cleanTitle(d.optString("title"));
          g.shortMsg = htmlText(d.optString("shortMessage"));
          g.msg = htmlText(d.optString("message"));
          g.begin = begin;
          g.end = end;
          out.put(did, g);
        }
        g.level = Math.max(g.level, level);
        if (!g.lines.contains(id)) g.lines.add(id);
      }
    }
    for (Incident g : out.values()) Collections.sort(g.lines, (a, b) -> lines.get(a).rank - lines.get(b).rank);
    return out;
  }

  /** « 20261120T110000 » (heure de Paris) -> instant, 0 si illisible */
  public static long parseCompact(String s) {
    if (s == null || s.length() < 15) return 0;
    try {
      return LocalDateTime.parse(s.substring(0, 15), COMPACT).atZone(PARIS).toInstant().toEpochMilli();
    } catch (DateTimeParseException e) {
      return 0;
    }
  }

  static String lineIdOf(String ref) {
    Matcher m = LINE_ID.matcher(ref == null ? "" : ref);
    return m.find() ? m.group(1) : "";
  }

  /** Texte brut d'un message PRIM (HTML) : sauts de ligne gardés, balises retirées, entités décodées */
  public static String htmlText(String s) {
    if (s == null || s.isEmpty()) return "";
    String t = s.replaceAll("(?i)<br\\s*/?>", "\n").replaceAll("(?i)</(p|li|div|h\\d)>", "\n").replaceAll("<[^>]+>", "");
    t = entities(t);
    t = t.replaceAll("[ \\t\\u00a0]+", " ").replaceAll(" *\n *", "\n").replaceAll("\n{3,}", "\n\n");
    return t.trim();
  }

  public static String cleanTitle(String s) { return TITLE_PREFIX.matcher(htmlText(s)).replaceFirst(""); }

  private static final String[][] ENT = {
      {"nbsp", " "}, {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""}, {"apos", "'"}, {"laquo", "«"}, {"raquo", "»"},
      {"eacute", "é"}, {"egrave", "è"}, {"ecirc", "ê"}, {"euml", "ë"}, {"agrave", "à"}, {"acirc", "â"}, {"ccedil", "ç"}, {"ocirc", "ô"},
      {"ugrave", "ù"}, {"ucirc", "û"}, {"icirc", "î"}, {"iuml", "ï"}, {"oelig", "œ"}, {"Eacute", "É"}, {"Egrave", "È"}, {"Agrave", "À"},
      {"Ccedil", "Ç"}, {"rsquo", "’"}, {"lsquo", "‘"}, {"ldquo", "“"}, {"rdquo", "”"}, {"hellip", "…"}, {"ndash", "–"}, {"mdash", "—"},
      {"euro", "€"}, {"deg", "°"}, {"middot", "·"}, {"bull", "•"}};

  private static String entities(String t) {
    if (t.indexOf('&') < 0) return t;
    StringBuffer out = new StringBuffer();
    Matcher m = Pattern.compile("&(#x[0-9a-fA-F]+|#\\d+|[A-Za-z]+);").matcher(t);
    while (m.find()) {
      String e = m.group(1), rep = null;
      try {
        if (e.startsWith("#x")) rep = new String(Character.toChars(Integer.parseInt(e.substring(2), 16)));
        else if (e.startsWith("#")) rep = new String(Character.toChars(Integer.parseInt(e.substring(1))));
        else for (String[] x : ENT) if (x[0].equals(e)) { rep = x[1]; break; }
      } catch (RuntimeException ignored) { /* entité inconnue : gardée telle quelle */ }
      m.appendReplacement(out, Matcher.quoteReplacement(rep == null ? m.group(0) : rep));
    }
    m.appendTail(out);
    return out.toString();
  }

  static String read(InputStream in) throws IOException {
    ByteArrayOutputStream b = new ByteArrayOutputStream();
    byte[] buf = new byte[16384];
    int n;
    while ((n = in.read(buf)) > 0) b.write(buf, 0, n);
    return new String(b.toByteArray(), StandardCharsets.UTF_8);
  }
}
