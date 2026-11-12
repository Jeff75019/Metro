#!/usr/bin/env bash
# KeoLive pour Android : compilation de dist/KeoLive.apk sans Gradle, avec les outils du SDK
# (aapt2, d8, zipalign, apksigner) et le compilateur Java du système. Aucune bibliothèque
# tierce : la coque native n'utilise que l'API Android.
#
#   ./build.sh                 compile et signe dist/KeoLive.apk
#   ANDROID_SDK=...            emplacement du SDK (par défaut ../../toolchain/android)
#   KEOLIVE_KEYSTORE=...       clé de signature (par défaut keystore/keolive.jks, alias « keolive »)
#   KEOLIVE_KEYSTORE_PASS=...  son mot de passe (sinon lu dans keystore/keolive.pass)
#
# La clé de signature n'est pas dans le dépôt (elle est propre au propriétaire : Android n'accepte
# une mise à jour que signée avec la même clé). Si elle manque, une nouvelle est créée, avec un mot
# de passe aléatoire écrit dans keystore/keolive.pass ; l'APK obtenu s'installera, mais pas
# par-dessus une version signée avec une autre clé (désinstaller d'abord).
#
# Dépendances : JDK 17 ou plus, python3 (Pillow pour les icônes), SDK Android
# (platforms;android-35, build-tools;35.0.0).
set -euo pipefail
cd "$(dirname "$0")"

SDK=${ANDROID_SDK:-$(cd .. && pwd)/../toolchain/android}
BT=$SDK/build-tools/35.0.0
PLATFORM=$SDK/platforms/android-35/android.jar
for f in "$BT/aapt2" "$BT/d8" "$BT/zipalign" "$BT/apksigner" "$PLATFORM"; do
  [ -e "$f" ] || { echo "introuvable : $f (SDK Android : $SDK)" >&2; exit 1; }
done

KS=${KEOLIVE_KEYSTORE:-keystore/keolive.jks}
PASSFILE=${KS%.jks}.pass
if [ ! -e "$KS" ]; then
  mkdir -p "$(dirname "$KS")"
  PASS=$(head -c 24 /dev/urandom | base64 | tr -d '/+=')
  printf '%s\n' "$PASS" > "$PASSFILE"
  chmod 600 "$PASSFILE"
  keytool -genkeypair -keystore "$KS" -alias keolive -keyalg RSA -keysize 2048 -validity 10000 \
    -storepass "$PASS" -keypass "$PASS" -dname "CN=KeoLive, O=Jean-Francois LAMBIN, C=FR" 2>/dev/null
  echo "nouvelle clé de signature : $KS (mot de passe dans $PASSFILE) — à garder précieusement" >&2
fi
PASS=${KEOLIVE_KEYSTORE_PASS:-$(cat "$PASSFILE")}

rm -rf build
mkdir -p build/assets/www/fonts build/gen build/classes build/dex dist

# 1. La page : interface mobile (web/), données et polices communes avec la version Windows
cp web/index.html web/style.css web/app.js build/assets/www/
cp ../windows/app/data.js ../windows/app/keolis.png build/assets/www/
cp ../windows/app/fonts/*.woff2 build/assets/www/fonts/
python3 tools/lines_json.py ../windows/app/data.js build/assets/lines.json

# 2. Ressources Android (icônes, thème, textes) et manifeste -> APK de base + R.java
"$BT/aapt2" compile --dir res -o build/res.zip
"$BT/aapt2" link -o build/base.apk -I "$PLATFORM" --manifest AndroidManifest.xml -A build/assets \
  --java build/gen --auto-add-overlay build/res.zip

# 3. Code Java -> classes.dex
javac --release 17 -encoding UTF-8 -Xlint:-options -cp "$PLATFORM" -d build/classes \
  build/gen/fr/jflambin/keolive/R.java $(find src -name '*.java' | sort)
"$BT/d8" --release --lib "$PLATFORM" --min-api 26 --output build/dex $(find build/classes -name '*.class' | sort)

# 4. Assemblage, alignement, signature
cp build/base.apk build/unsigned.apk
python3 - build/unsigned.apk build/dex/classes.dex <<'EOF'
import sys, zipfile
with zipfile.ZipFile(sys.argv[1], 'a', zipfile.ZIP_DEFLATED) as z:
    z.write(sys.argv[2], 'classes.dex')
EOF
"$BT/zipalign" -f -p 4 build/unsigned.apk build/aligned.apk
"$BT/apksigner" sign --ks "$KS" --ks-key-alias keolive \
  --ks-pass "pass:$PASS" --key-pass "pass:$PASS" --out dist/KeoLive.apk build/aligned.apk
"$BT/apksigner" verify dist/KeoLive.apk
(cd dist && sha256sum KeoLive.apk > KeoLive.apk.sha256)
ls -la dist/KeoLive.apk
