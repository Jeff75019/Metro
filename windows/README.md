# KeoLive

**KeoLive** est un tableau de bord des prochains passages en Île-de-France pour Windows 10 et 11, dans l'esprit des écrans de quai : très grands chiffres, couleurs des lignes, thème sombre ou clair.

![Tableau de bord](apercus/tableau.png)

- **Recherche libre :** n'importe quelle station de métro, de RER ou de tram, une gare SNCF ou un arrêt de bus (`Ctrl K`). Un aperçu s'ouvre, et le bouton **Épingler** l'ajoute au tableau.
- **Tableau :** les arrêts épinglés côte à côte, avec les minutes avant chaque passage, les missions et les voies du RER, « À QUAI », « APPROCHE » et « SUPPRIMÉ ». On les réordonne en les faisant glisser par leur titre.
- **Grandes gares de Paris :** une carte, présente d'office sur le tableau, avec un onglet par gare (Lyon, Nord, Est, Montparnasse, Saint-Lazare, Austerlitz, Bercy). On y voit les départs ou les arrivées des grandes lignes : heure, destination ou provenance, train, voie, retard ou suppression. Elle ne demande pas de clé.
- **Autres gares SNCF :** n'importe quelle gare, par la recherche, avec les mêmes informations.
- **État du trafic :** toutes les lignes de métro, de RER, de Transilien et de tram, avec une pastille de couleur par ligne. Les lignes 16 et 17, en construction, y figurent en pointillés avec leur année d'ouverture prévue et leurs stations. Un clic donne le détail des messages. Un bandeau défilant rappelle les perturbations en cours.
- **Alertes d'incident :** dès qu'un incident touche une ligne (trafic perturbé ou interrompu), une fenêtre s'ouvre au centre de l'application, avec un carillon, et donne les lignes concernées, le titre, la période et le message complet. Elle se met à jour si l'incident s'aggrave et annonce le retour à la normale. Si KeoLive n'est pas au premier plan, Windows affiche une notification ; un clic dessus ouvre la fiche de la ligne. Voir « Alertes » plus bas.
- **À propos (bouton ⓘ, ou clic sur le logo) :** version de l'application, propriétaire et entreprise.
- **Thème clair ou sombre (`L`) :** le bouton soleil ou lune de l'en-tête bascule entre les deux. Dans les réglages, « Automatique » suit le thème de Windows.
- **Plein écran (`F11`) :** tout l'écran, par-dessus la barre des tâches, sans bordure ni barre de titre (bouton à quatre coins de l'en-tête), pour un écran dédié. Le curseur se cache après 3 secondes sans bouger la souris. On en sort par `F11`, `Échap` ou le même bouton. Si vous fermez l'application en plein écran, elle se rouvre ainsi.
- **Mode compact (`M`) :** une petite fenêtre en haut à droite de l'écran, une carte à la fois (flèches ← →). L'option « toujours au premier plan » (`T`) la garde visible par-dessus vos autres fenêtres.

| Thème clair | Recherche | Détail d'une ligne |
|---|---|---|
| ![Thème clair](apercus/clair.png) | ![Recherche](apercus/recherche.png) | ![Détail](apercus/ligne.png) |

| À propos | Mode compact |
|---|---|
| ![À propos](apercus/apropos.png) | ![Compact](apercus/compact.png) |

## Installation

1. Téléchargez [`KeoLive.exe`](https://github.com/Jeff75019/Metro/raw/claude/esp32-metro-display-app-72ncxo/windows/dist/KeoLive.exe) (environ 1 Mo, un seul fichier, sans installation ni droits d'administrateur).
2. Avant le premier lancement, **débloquez le fichier** : clic droit sur `KeoLive.exe` → **Propriétés** → en bas de l'onglet Général, cochez **Débloquer** → **OK**. Windows ne l'affichera plus comme suspect (voir plus bas). Double-cliquez ensuite dessus.
3. Collez votre **clé PRIM** dans le bandeau jaune (voir ci-dessous), puis cherchez une station.

L'application utilise WebView2, le moteur d'Edge, présent d'office dans Windows 11. Les réglages et la position de la fenêtre sont gardés dans `%LOCALAPPDATA%\KeoLive`.

**Sans `.exe` :** si votre poste de travail interdit les programmes téléchargés, ouvrez [`KeoLive.html`](dist/KeoLive.html) dans Edge. C'est la même application, dans le navigateur, sans le mode « premier plan ».

## Pourquoi Windows se méfie, et comment l'éviter

Windows (SmartScreen) avertit pour tout programme **téléchargé** qui n'est pas **signé** par un éditeur connu. Cela ne veut pas dire qu'un virus a été trouvé : Windows ne connaît simplement pas ce programme. Trois solutions, de la plus simple à la plus complète :

- **Débloquer le fichier** (pour vous, une fois pour toutes) : clic droit → Propriétés → cocher **Débloquer**. Ou, dans PowerShell, dans le dossier du fichier : `Unblock-File .\KeoLive.exe`. Windows retire alors la marque « téléchargé depuis Internet » de ce fichier. Sur un poste d'entreprise, ce réglage peut être interdit par le service informatique : il peut alors autoriser le programme, ou vous pouvez utiliser `KeoLive.html` dans Edge.
- **Cliquer sur « Exécuter quand même »** dans l'avertissement (Informations complémentaires → Exécuter quand même) : Windows ne redemande pas pour ce fichier.
- **Faire disparaître l'avertissement pour tout le monde** : signer l'exécutable avec un certificat de signature de code (payant, délivré après vérification d'identité, par exemple via le service de signature de Microsoft), ou publier l'application dans le Microsoft Store (compte développeur Microsoft). Même signé, un programme peu téléchargé peut encore être signalé quelque temps, le temps que sa réputation s'établisse.

## Clé PRIM (gratuite)

Les passages en temps réel et l'état du trafic viennent de PRIM, la plateforme d'Île-de-France Mobilités. Les trains SNCF n'ont pas besoin de clé.

1. Créez un compte sur [prim.iledefrance-mobilites.fr](https://prim.iledefrance-mobilites.fr).
2. Abonnez-vous aux API **« Prochains passages (requête unitaire) »** et **« Messages Info Trafic »**.
3. Dans « Mon compte », générez un jeton, puis collez-le dans l'application (bandeau jaune, ou ⚙ Réglages).

La même clé peut servir à l'afficheur ESP32 : le quota du jour est alors partagé.

## Alertes d'incident

![Alerte d'incident](apercus/alertes.png)

- **Au lancement :** aucune fenêtre ; les incidents déjà en cours sont dans la colonne Trafic et le bandeau défilant. Une fenêtre ne s'ouvre que lorsqu'une ligne devient perturbée pendant que KeoLive tourne.
- **Où :** au centre de l'application, plus grande, avec un léger voile sur le tableau (on peut continuer à cliquer derrière).
- **Son :** un court carillon à chaque fenêtre : trois notes descendantes pour un incident, quatre pour une aggravation, deux notes montantes pour un trafic rétabli. Réglage « Son des alertes » dans ⚙ Réglages (avec « Écouter » pour l'essayer). La notification de Windows, elle, reste silencieuse pour ne pas sonner deux fois.
- **Ensuite :** une fenêtre par nouvel incident, puis « Aggravation » s'il passe de perturbé à interrompu, et « Trafic rétabli » à la fin. Une fenêtre se ferme seule au bout d'environ une minute (elle attend tant que la souris est dessus) ; « Voir la ligne » ouvre tous les messages de la ligne.
- **Quelles lignes :** dans ⚙ Réglages, « Alertes d'incident » : **Toutes** (par défaut), **Mes lignes** ou **Aucune**. « Mes lignes » regroupe les lignes des arrêts épinglés et celles que vous suivez : ouvrez la fiche d'une ligne (clic sur sa pastille dans la colonne Trafic) et cliquez sur **Suivre cette ligne**.
- **En arrière-plan :** fenêtre réduite ou cachée derrière d'autres, KeoLive continue de lire l'info trafic et prévient par une notification de Windows ; son bouton clignote dans la barre des tâches. Une icône KeoLive reste près de l'horloge : un clic dessus ramène la fenêtre.
- **Essai :** `KeoLive.exe --essai-notification` affiche une notification d'essai pendant 8 secondes, sans ouvrir l'application. Si rien n'apparaît, les notifications sont coupées dans Windows (Paramètres → Système → Notifications, ou mode « Ne pas déranger »).
- **Délai :** l'info trafic est relue toutes les 2 minutes quand les alertes sont actives. Une alerte arrive donc au plus 2 minutes après la publication de l'incident par Île-de-France Mobilités.

## Quota et rafraîchissement

- Chaque arrêt est mis à jour toutes les 60 secondes par défaut (30, 60 ou 120 s dans les réglages).
- L'application compte ses appels du jour. Si le quota risque de manquer avant 20 h, elle ralentit d'elle-même, en répartissant ce qui reste sur la journée.
- Quand la fenêtre est réduite, les passages ne sont plus demandés ; seule l'info trafic continue, pour les alertes.
- Le quota par défaut est de 1000 appels par jour ; mettez celui de votre abonnement PRIM dans les réglages. L'info trafic est relue toutes les 2 minutes (5 minutes si les alertes sont coupées), sur son propre quota, y compris fenêtre réduite.

## Raccourcis

| Touche | Action |
|---|---|
| `Ctrl K` ou `/` | rechercher |
| `↑` `↓` `Entrée` | choisir un résultat |
| `F11` | plein écran (par-dessus la barre des tâches) |
| `Échap` | quitter le plein écran, fermer une fenêtre |
| `M` | mode compact / tableau complet |
| `←` `→` | carte précédente / suivante (mode compact) |
| `T` | toujours au premier plan |
| `L` | thème clair / sombre |
| `F5` | actualiser |
| `Ctrl` + molette | agrandir ou réduire l'affichage |

## Au travail : réseau et pare-feu

L'application passe par la connexion de Windows : le proxy de l'entreprise est suivi comme dans Edge. Elle n'interroge que ces adresses :

- `prim.iledefrance-mobilites.fr` : passages et info trafic ;
- `data.iledefrance-mobilites.fr` : recherche des arrêts de bus, noms et couleurs des lignes de bus ;
- `ressources.data.sncf.com` : recherche des gares SNCF ;
- `api.trainnomade.fr` : départs et arrivées SNCF.

Si une carte affiche « PRIM injoignable (réseau ou pare-feu) », c'est que le réseau de l'entreprise bloque l'une de ces adresses : demandez au service informatique de l'autoriser.

## Pour les développeurs

```
app/            l'application (HTML, CSS, JavaScript, polices Barlow sous licence OFL)
  data.js       lignes, stations et gares, générées depuis le projet (tools/)
native/         la fenêtre Windows (C++, WebView2) : icône, premier plan, mode compact
tools/          bundle.py (application en un seul fichier), make_data.py, dump_reseau.cpp
dist/           KeoLive.exe et KeoLive.html, prêts à l'emploi
```

- **Compiler sous Linux :** `make`. Il faut MinGW-w64 (`g++-mingw-w64-x86-64-posix`), la bibliothèque [webview](https://github.com/webview/webview) 0.12 et le SDK WebView2 (paquet NuGet `Microsoft.Web.WebView2`). Les chemins sont en tête du `Makefile`.
- **Essayer l'interface dans un navigateur :** servez la racine du dépôt (`python3 -m http.server`) et ouvrez `windows/app/index.html`. Avec `?demo`, les réponses PRIM sont remplacées par des réponses réelles enregistrées (`test/data/`) et remises à l'heure.
- **Mettre à jour les stations :** `g++ -I../Metro -I<ArduinoJson>/src tools/dump_reseau.cpp ../Metro/reseau.cpp ../Metro/lines.cpp ../Metro/siri.cpp -o dump`, puis `./dump | python3 tools/make_data.py ../Metro/lines.cpp > app/data.js`.

## Limites

- **Pas encore lancée sur un vrai Windows.** L'interface a été essayée dans Chromium, le moteur de WebView2, avec des réponses PRIM réelles enregistrées et les données SNCF en direct. L'exécutable a été démarré sous Wine (sans WebView2) : il prépare la page et affiche correctement le message prévu quand le moteur manque. Le premier essai réel se fera sur votre PC.
- PRIM est protégé par un filtre anti-robots (Cloudflare). Depuis un poste normal, il laisse passer l'application comme le navigateur ; s'il bloquait un réseau d'entreprise, les cartes afficheraient « PRIM injoignable ».
- Données : Île-de-France Mobilités (PRIM, open data), SNCF (ODbL), Train Nomade.
