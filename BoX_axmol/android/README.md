# Werbung (Google AdMob) im Android-Build

Banner im Menü + Vollbild-Anzeige (Interstitial) nach jedem 3. gewonnenen Level. Auf Desktop/iOS passiert nichts (leere Aufrufe).

## Dateien
- `ads-android.patch` – Änderungen am Axmol-Android-Projekt (`proj.android`, aus dem Template): `app/build.gradle` (AdMob-Abhängigkeit),
  `app/AndroidManifest.xml` (App-ID + Netzwerk-Berechtigung), `app/proguard-rules.pro` (`AdManager` behalten), `AppActivity.java`
  (`AdManager.init(this)`) und die neue Datei `app/src/dev/axmol/app/AdManager.java`.
- `AdManager.java` – dieselbe Datei einzeln (zum Ansehen/Kopieren).
- `../Source/Ads.h` – C++-Seite (JNI-Brücke). Schalter oben in der Datei: `kEnabled`, `kBanner`, `kInterstitial`, `kLevelsPerInterstitial`.

## Einbauen
```bash
cd <dein-projekt>/proj.android
git apply /pfad/zu/BoX_axmol/android/ads-android.patch   # oder die vier Änderungen von Hand übernehmen
```
Weicht dein `proj.android` vom Template ab, übernimm die Änderungen von Hand: `AdManager.java` nach `app/src/dev/axmol/app/` kopieren, in
`AppActivity.onCreate` am Ende `AdManager.init(this);` aufrufen, die Gradle-Zeile `implementation 'com.google.android.gms:play-services-ads:24.2.0'`
(bei Bedarf aktuelle Version), die `meta-data` mit der App-ID und die Proguard-Zeile `-keep class dev.axmol.app.AdManager { *; }` eintragen.
Der Package-Name `dev.axmol.app` muss zu deiner `AppActivity` passen.

## Wichtig vor der Veröffentlichung
- Im Code stehen **Googles Test-IDs** (App-ID im Manifest, Banner- und Interstitial-ID in `AdManager.java`). Sie zeigen Test-Anzeigen. Ersetze sie durch
  deine eigenen IDs aus der AdMob-Konsole, sonst gibt es kein Geld – und zeige mit eigenen IDs nie Echt-Anzeigen auf deinem eigenen Gerät (Testgerät eintragen).
- **Einwilligung (DSGVO):** `AdManager` fragt über Googles UMP ab, bevor Werbung geladen wird. Das Formular musst du in der AdMob-Konsole unter
  „Datenschutz & Mitteilungen" einrichten, sonst erscheint keines.
- Google Play Console: „Enthält Werbung" ankreuzen und das Formular zur Datensicherheit ausfüllen. Richtet sich das Spiel an Kinder, gelten
  zusätzliche Regeln (Families Policy, Altersangabe für Anzeigen) – dann muss `AdRequest`/`RequestConfiguration` angepasst werden.
- `play-services-ads` verlangt eine Mindest-SDK-Version (aktuell ab Android 6 / API 23). Prüfe `minSdk` in deinen Build-Profilen.
- Der Banner sitzt unten und ist nur im Menü sichtbar; im Spiel und im Editor wird er ausgeblendet, damit er die Touch-Tasten nicht verdeckt.
  Im Querformat (Template-Standard `sensorLandscape`) kann er unten in das Menü-Quadrat ragen.

## Nicht getestet
Dieser Code wurde ohne Android-SDK geschrieben und nur syntaktisch geprüft (C++ gegen die Axmol-Header, Java ohne die Google-Bibliotheken).
Bau ihn einmal mit den Test-IDs und prüfe `adb logcat | grep BoXAds`.
