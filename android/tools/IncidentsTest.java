import fr.jflambin.keolive.Incidents;

import java.io.FileInputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.time.Instant;
import java.util.Map;
import java.util.Set;

/** Essai sur un ordinateur du décodage de l'info trafic (sans Android) :
 *    javac -cp json.jar -d out src/fr/jflambin/keolive/Incidents.java tools/IncidentsTest.java
 *    java -cp json.jar:out IncidentsTest build/assets/lines.json ../test/data/prim_infotrafic_reel.json [lignes] */
public class IncidentsTest {
  public static void main(String[] a) throws Exception {
    Map<String, Incidents.Line> lines;
    try (FileInputStream in = new FileInputStream(a[0])) { lines = Incidents.loadLines(in); }
    String body = new String(Files.readAllBytes(Paths.get(a[1])), "UTF-8");
    String upd = new org.json.JSONObject(body).optString("lastUpdatedDate", "");
    long now = upd.isEmpty() ? System.currentTimeMillis() : Instant.parse(upd).toEpochMilli();
    Set<String> only = a.length > 2 ? new java.util.HashSet<>(java.util.Arrays.asList(a[2].split(","))) : null;
    Map<String, Incidents.Incident> inc = Incidents.parse(body, lines, now, only);
    System.out.println("instant : " + upd + " ; incidents en cours (perturbé ou interrompu) : " + inc.size());
    for (Incidents.Incident g : inc.values()) {
      System.out.println("- [" + (g.level == 3 ? "INTERROMPU" : "PERTURBÉ") + (g.works ? ", travaux" : "") + "] " + g.lines + " " + g.title
          + " | " + g.shortMsg + " | " + g.msg.replace("\n", " ⏎ ").substring(0, Math.min(90, g.msg.length())));
    }
    // texte des messages
    String h = "<p>Le trafic est <b>interrompu</b> entre Ch&acirc;telet &amp; Nation.<br/>Reprise &agrave; 09h30.</p><ul><li>Ligne 14</li></ul>";
    System.out.println("htmlText : " + Incidents.class.getDeclaredMethod("htmlText", String.class).invoke(null, h).toString().replace("\n", "⏎"));
    System.out.println("cleanTitle : " + Incidents.class.getDeclaredMethod("cleanTitle", String.class).invoke(null, "Métro 1 : Colis suspect - Trafic perturbé"));
    System.out.println("parseCompact : " + Incidents.class.getDeclaredMethod("parseCompact", String.class).invoke(null, "20261120T110000")
        + " (attendu 1795168800000 = 2026-11-20 10:00 UTC, soit 11:00 à Paris)");
  }
}
