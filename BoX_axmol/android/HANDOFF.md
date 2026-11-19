# Übergabe: Android-Werbung (AdMob) für BoX – Stand der Cloud-Sitzung

Zweck: Die lokale Sitzung im Projekt `D:\_git\AxmolHub\data\projects\BoXAx` soll die Werbung in das echte Projekt einbauen und den Build zum Laufen bringen.

## Schon gemacht (alles in diesem Repository, Branch `claude/wonderful-newton-jmoprw`, Ordner `BoX_axmol/`)
- `android/AdManager.java` (Package `dev.axmol.app`): Banner (unten, `AdView`), Interstitial (vorladen, bei Bedarf zeigen, nach dem Schließen neu laden),
  Einwilligung per Google UMP (`requestConsentInfoUpdate` + `loadAndShowConsentFormIfRequired`), erst danach `MobileAds.initialize`.
  Statische Methoden `init(Activity)`, `showBanner()`, `hideBanner()`, `showInterstitial()`; alles läuft auf dem UI-Thread, beliebige Threads dürfen aufrufen.
  Eingetragen sind Googles **Test-IDs** (Banner `ca-app-pub-3940256099942544/6300978111`, Interstitial `…/1033173712`).
- `android/ads-android.patch` (gegen das Axmol-Template `proj.android`; `git apply --check` gegen das Template ist erfolgreich):
  - `app/build.gradle`: `implementation 'com.google.android.gms:play-services-ads:24.2.0'`
  - `app/AndroidManifest.xml`: `meta-data com.google.android.gms.ads.APPLICATION_ID` (Test-App-ID `ca-app-pub-3940256099942544~3347511713`) + Permission `ACCESS_NETWORK_STATE`
  - `app/proguard-rules.pro`: `-keep class dev.axmol.app.AdManager { *; }` (nötig, weil per JNI aufgerufen und das Release `minifyEnabled true` hat)
  - `AppActivity.java`: `AdManager.init(this);` am Ende von `onCreate` (unter „DO OTHER INITIALIZATION BELOW")
  - neue Datei `app/src/dev/axmol/app/AdManager.java`
- C++: `Source/Ads.h` (header-only, JNI-Brücke `ax::JniHelper::callStaticVoidMethod("dev.axmol.app.AdManager", …)`; auf Nicht-Android leere Aufrufe).
  Schalter dort: `kEnabled`, `kBanner`, `kInterstitial`, `kLevelsPerInterstitial = 3`.
- Einbindung in `Source/BoxScene.cpp`: `#include "Ads.h"`; in `update()` Banner nur im Menü (`State::Menu`), im Spiel/Editor ausgeblendet (Member `_bannerShown`);
  im Gewinn-Zweig vor `startLevel(_level + 1)` ein Interstitial nach jedem 3. gewonnenen Level, nie im Editor-Testspiel (Member `_adCounter`).
- `android/README.md` beschreibt Einbau und Richtlinien.

## Geprüft / nicht geprüft
- Geprüft: C++ gegen die Axmol-Header (auch der Android-Zweig von `Ads.h` mit simulierten JNI-/Android-Headern); der Patch passt auf das Template.
- **Nicht geprüft:** das Java (nur auf Syntaxfehler gelesen, nie gegen die Google-Bibliotheken übersetzt), der Gradle-Build, ein Lauf auf einem Gerät.

## Offen / zu tun in der lokalen Sitzung
1. Patch auf das echte `proj.android` anwenden. Das Projekt weicht eventuell vom Template ab (Package `dev.axmol.boxax`? – der Log zeigt `dev.axmol.boxax/dev.axmol.app.AppActivity`,
   also bleibt `AppActivity` in `dev.axmol.app`, die `applicationId` ist `dev.axmol.boxax`). Dateien und `build.gradle` des Nutzers prüfen und die Änderungen von Hand anpassen, falls `git apply` scheitert.
2. **Installation scheitert:** `INSTALL_PARSE_FAILED_NO_CERTIFICATES` – AxmolHub baut `app-release-unsigned.apk`. Lösung: Debug bauen, oder `signingConfig signingConfigs.debug` im Release-Block (nur Tests),
   oder echten Schlüssel (`keytool -genkeypair …`) + `KEY_STORE_FILE`, `KEY_STORE_PASSWORD`, `KEY_ALIAS`, `KEY_PASSWORD` in `gradle.properties` (Passwörter nicht ins Repository).
3. `play-services-ads` 24.2.0 ist eine Schätzung aus dem Kenntnisstand der Cloud-Sitzung – gegen die aktuelle Version prüfen; `minSdk` muss zu dieser Version passen.
4. Banner sitzt unten und kann im Querformat (Template: `sensorLandscape`) in das Menü-Quadrat (640×640 Design-Auflösung, SHOW_ALL) ragen.
5. Vor der Veröffentlichung: Test-IDs durch eigene AdMob-IDs ersetzen (App-ID im Manifest, Banner-/Interstitial-ID in `AdManager.java`), UMP-Formular in der AdMob-Konsole einrichten,
   in der Play Console „Enthält Werbung" + Datensicherheit angeben. Nicht von selbst ersetzen – die echten IDs hat nur der Nutzer.

## Sonstiger Spielstand (zur Orientierung)
Quellen in `Source/` (`BoxScene.cpp/.h`, `MetaballMesh.h`, `Ads.h`, `AppDelegate.cpp/.h`), Sounds in `Content/sfx/`. Zuletzt geändert: Tunnelschutz für Tropfen
(`kStepsPerFrame = 6`, `kMaxW = 4`, `kWallR = 4.5 px`, `guardWalls()`), Build-Stempel unten links im Menü (`kBuildStamp`) zum Vergleich Desktop/Android.
