# Afficheur Métro

Un petit afficheur de poche qui indique, en temps réel, l'attente des **deux prochains passages** de votre métro, RER, tram ou bus en Île-de-France, comme les écrans des quais.

Matériel : **Waveshare ESP32-S3-Touch-LCD-1.9** (écran tactile 1,9" 170×320) et une batterie LiPo.
Données : **API PRIM d'Île-de-France Mobilités** (gratuite, clé personnelle).

![Aperçu des écrans](docs/apercu.png)

*Rendus générés sur PC à partir du vrai code d'affichage (voir « Tests sur PC »).*

La configuration se fait depuis le téléphone :

![Page de configuration](docs/configuration.png)

## Fonctionnalités

- Deux prochains passages avec destination et attente en minutes, rafraîchis automatiquement.
- Mentions « À quai », « À l'approche » (clignotant), « Supprimé », « Retardé », voie (RER), « Horaire théorique » quand l'info n'est pas en temps réel.
- Pictogrammes de lignes aux couleurs officielles (métro, RER, Transilien, tram) ; bus et autres lignes acceptés.
- Jusqu'à **4 arrêts favoris** (glisser pour passer de l'un à l'autre), avec choix du sens.
- **Configuration depuis le téléphone** : l'afficheur crée son propre WiFi (QR code à l'écran). La page web permet de chercher une station par son nom et de choisir la ligne et le sens.
- Polices anti-aliasées avec tous les accents (« Château », « Défense », « Église »...).
- **Batterie** : niveau affiché, veille profonde après 60 s, réveil par le bouton BOOT (ou en secouant l'afficheur, expérimental). Au réveil, les derniers horaires s'affichent immédiatement pendant la reconnexion.
- Jusqu'à 3 réseaux WiFi (maison, bureau, partage de connexion du téléphone).
- Respect du quota PRIM (compteur d'appels par jour).

## 1. Obtenir une clé API PRIM (gratuit)

1. Créez un compte sur [prim.iledefrance-mobilites.fr](https://prim.iledefrance-mobilites.fr).
2. Dans le catalogue, ouvrez l'API **« Prochains passages (plateforme Île-de-France Mobilités) - requête unitaire »** et abonnez-vous.
3. Dans « Mon compte » → « Mes jetons d'authentification », générez un jeton. C'est votre **clé API**.

> **Quota :** les comptes récents ont droit à **1000 appels par jour**. En usage « porte-clés » (on regarde l'écran 1 minute, il se rendort), on consomme 2 à 3 appels par consultation : c'est largement suffisant. Si vous voulez un affichage permanent, demandez une augmentation de quota à PRIM puis indiquez le nouveau quota dans les réglages.

## 2. Installer le programme

### Avec l'IDE Arduino (le plus simple)

1. Installez l'[IDE Arduino 2](https://www.arduino.cc/en/software).
2. **Fichier → Préférences → URL de gestionnaire de cartes supplémentaires**, ajoutez :
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. **Outils → Carte → Gestionnaire de cartes** : installez **esp32** par Espressif, **version 3.3.12** (3.0 minimum).
4. **Croquis → Inclure une bibliothèque → Gérer les bibliothèques** : installez
   - **LovyanGFX** (lovyan03), version 1.2.36 ou plus récente ;
   - **ArduinoJson** (Benoit Blanchon), version 7.x.
5. Ouvrez `Metro/Metro.ino`.
6. Réglez le menu **Outils** :

   | Réglage | Valeur |
   |---|---|
   | Carte | **ESP32S3 Dev Module** |
   | USB CDC On Boot | **Enabled** |
   | Flash Size | **16MB (128Mb)** |
   | Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** |
   | PSRAM | **OPI PSRAM** |
   | Upload Mode | UART0 / Hardware CDC |

7. Branchez la carte en USB-C et cliquez sur **Téléverser**.
   Si le téléversement échoue : maintenez **BOOT**, appuyez brièvement sur **RESET**, relâchez BOOT, puis recommencez.

### Sans rien installer : binaire précompilé

Si vous avez le fichier `metro-merged.bin` (image complète : chargeur de démarrage + table de partitions + programme) :

1. Ouvrez [espressif.github.io/esptool-js](https://espressif.github.io/esptool-js/) dans Chrome ou Edge.
2. Branchez la carte, cliquez sur **Connect** et choisissez le port USB (si rien n'apparaît : maintenez **BOOT**, appuyez sur **RESET**, relâchez BOOT).
3. Adresse **0x0**, fichier `metro-merged.bin`, puis **Program**.
4. Appuyez sur **RESET**.

En ligne de commande : `esptool.py --chip esp32s3 write_flash 0x0 metro-merged.bin`.

### Avec PlatformIO (VS Code)

Ouvrez le dossier du dépôt dans VS Code avec l'extension PlatformIO, puis :

```bash
pio run -t upload        # compile et téléverse
pio device monitor       # journal série (115200 bauds)
```

`platformio.ini` utilise la plateforme [pioarduino](https://github.com/pioarduino/platform-espressif32) (cœur Arduino ESP32 3.3.12) et télécharge les bibliothèques tout seul.

## 3. Configurer

Au premier démarrage, l'écran affiche un **QR code** et le nom d'un WiFi « Metro-XXXX ».

1. Scannez le QR code avec l'appareil photo du téléphone (ou rejoignez « Metro-XXXX » à la main). La page de configuration s'ouvre ; sinon, allez sur **http://192.168.4.1**.
2. **WiFi** : « Rechercher les réseaux », choisissez le vôtre, entrez le mot de passe, « Ajouter et se connecter ». Attendez « Connecté ✓ ».
3. **Clé API PRIM** : collez votre jeton.
4. **Mes arrêts** : tapez un nom de station (« Bastille »), choisissez-la, puis la ligne et, si vous voulez, le sens. Répétez pour 4 arrêts au plus.
5. **Enregistrer et redémarrer**.

Pour revenir plus tard à la configuration : **appui long (3 s) sur l'écran** ou **bouton BOOT maintenu 3 s**. Les réglages déjà enregistrés sont conservés ; mots de passe et clé ne sont jamais réaffichés (laisser le champ vide = garder l'ancien).

> La recherche d'arrêts et la liste des lignes passent par l'afficheur, qui doit donc d'abord être connecté à votre WiFi (étape 2).
> Pour une saisie manuelle, l'identifiant d'arrêt peut être un `MonitoringRef` PRIM (`STIF:StopArea:SP:71517:`), un identifiant open data (`IDFM:463158`, `IDFM:monomodalStopPlace:58566`) ou un simple numéro de zone d'arrêt. La ligne est l'identifiant IDFM (`C01371` pour la ligne 1).

## 4. Utilisation

| Geste | Effet |
|---|---|
| Toucher l'écran | Changer de sens (si l'arrêt en a plusieurs), sinon actualiser |
| Glisser vers la gauche / la droite | Arrêt favori suivant / précédent |
| Appui long sur l'écran (3 s) | Mode configuration |
| Bouton BOOT (appui court) | Réveiller / mettre en veille |
| Bouton BOOT maintenu 3 s | Mode configuration (ou en sortir) |

Le pied d'écran indique l'âge des données (« Mis à jour il y a 12 s »), le sens affiché (« sens 1/2 ») et l'arrêt courant (points).

### Modes de veille (page de configuration)

- **Veille profonde** (défaut, sur batterie) : après la durée d'affichage (60 s), tout s'éteint. Réveil par BOOT, ou par une secousse si l'option est cochée. Consommation de veille très faible.
- **Écran éteint** (sur secteur) : l'écran s'éteint, un toucher le rallume.
- **Toujours allumé** (sur secteur) : l'intervalle de rafraîchissement est automatiquement allongé pour tenir dans le quota du jour (≈ 1 appel toutes les 87 s avec 1000 appels/jour).

## Dépannage

| Symptôme | Solution |
|---|---|
| Image à l'envers | Réglages → « Retourner l'écran (180°) » |
| « Clé API refusée (401/403) » | Vérifiez le jeton et l'abonnement à l'API « Prochains passages (requête unitaire) » |
| « Quota PRIM dépassé (429) » | Quota du jour épuisé : attendre minuit, ou réduire le rafraîchissement |
| « Aucun passage prévu » | Service terminé, ou temps réel indisponible sur cette ligne |
| « Horaire théorique » | IDFM ne fournit pas de temps réel pour ce passage, c'est l'horaire prévu |
| Le WiFi n'est pas trouvé | La carte ne gère que le WiFi 2,4 GHz |
| Écran noir après téléversement | PSRAM mal réglée : choisir **OPI PSRAM** |

Journal de fonctionnement : moniteur série à 115200 bauds.

## Organisation du code

```
Metro/                  sketch Arduino (aussi le src_dir de PlatformIO)
├── Metro.ino           programme principal : WiFi, rafraîchissement, gestes, veille
├── config.h            broches de la carte et valeurs par défaut
├── board.h/.cpp        écran + tactile (LovyanGFX), batterie, IMU, veille profonde
├── siri.h/.cpp         décodage des réponses PRIM (SIRI Lite), choix des 2 passages
├── prim.h/.cpp         requêtes HTTPS : PRIM, recherche d'arrêts, quota
├── ui.h/.cpp           dessin des écrans (tampon mémoire, sans scintillement)
├── lines.h/.cpp        lignes connues : nom, couleurs officielles
├── settings.h/.cpp     réglages (JSON en mémoire flash)
├── portal.h/.cpp       point d'accès WiFi, portail captif, API de la page web
├── portal_html.h       la page de configuration
└── fonts_metro.h       polices générées (ne pas modifier à la main)
tools/make_fonts.py     génère fonts_metro.h à partir de tools/fonts/
tools/portal_preview.py aperçu de la page de configuration sur PC (données fictives)
tools/idf-build/        compilation de vérification avec ESP-IDF
test/                   tests sur PC (décodage + rendu des écrans en images)
```

### Brochage de la carte (Waveshare ESP32-S3-Touch-LCD-1.9)

| Fonction | GPIO |
|---|---|
| LCD ST7789V2 : SCLK, MOSI, CS, DC, RST | 10, 13, 12, 11, 9 |
| Rétroéclairage (actif à l'état bas) | 14 |
| I2C (tactile CST816S 0x15, IMU QMI8658 0x6B) : SDA, SCL | 47, 48 |
| Tactile : INT, RST | 21, 17 |
| Tension batterie (pont ÷3) | 4 |
| Bouton BOOT | 0 |

## Tests sur PC

Le décodage des réponses PRIM et le dessin des écrans se compilent aussi sur un PC Linux ou macOS, sans la carte :

```bash
cd test
make test   # tests du décodage (réponse d'exemple dans test/data/)
make ui     # rendu des écrans dans test/out/*.png
```

Il faut `g++` et les sources de ArduinoJson et LovyanGFX (chemins réglables : `make ARDUINOJSON=... LOVYANGFX=... ui`), plus Pillow (`pip install pillow`) pour les PNG.

Pour modifier les polices (taille, caractères) : `pip install pillow && python3 tools/make_fonts.py`.

Pour retoucher la page de configuration sans la carte : `python3 tools/portal_preview.py`, puis ouvrez http://localhost:8080 (données fictives, rien n'est envoyé).

## État et limites

- Le décodage PRIM (tests avec sanitizers) et l'affichage (rendus ci-dessus) ont été vérifiés sur PC.
- Le firmware complet **compile sans erreur ni avertissement** (`-Wall -Wextra`) pour l'ESP32-S3 avec ESP-IDF 5.5.6 + cœur Arduino 3.3.12 + LovyanGFX 1.2.36 + ArduinoJson 7.4.3 (binaire d'environ 1,5 Mo, la moitié de la partition). Il **n'a pas encore été essayé sur la carte** : au premier essai, regardez le moniteur série et signalez les erreurs.
- Le **réveil par secousse** est expérimental : la séquence de l'IMU vient de notes de la communauté, et la broche d'interruption n'est pas confirmée. Si l'afficheur se réveille tout seul dans une poche, il suspend ce réveil 30 minutes après 3 réveils sans utilisation.
- Le sens d'affichage (paysage, USB à gauche ou à droite) dépend du montage : utilisez l'option « Retourner l'écran ».
- Les connexions HTTPS vérifient les certificats avec le magasin intégré au cœur ESP32 (versions 3.3.x). Si la connexion échoue, l'afficheur refait un essai sans vérification pour ne pas rester bloqué.

## Licences et sources

- Polices : [Barlow Semi Condensed et Barlow Condensed](https://github.com/jpt/barlow), SIL Open Font License 1.1 (`tools/fonts/OFL.txt`).
- Données : API PRIM et open data d'Île-de-France Mobilités ; couleurs des lignes issues du référentiel des lignes IDFM.
- Bibliothèques : [LovyanGFX](https://github.com/lovyan03/LovyanGFX) (FreeBSD), [ArduinoJson](https://arduinojson.org) (MIT), [cœur Arduino ESP32](https://github.com/espressif/arduino-esp32) (LGPL 2.1).
- Projet indépendant, sans lien avec la RATP, IDFM ou les fabricants d'afficheurs du commerce.
