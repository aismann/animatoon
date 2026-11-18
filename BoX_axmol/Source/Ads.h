#pragma once
// Werbung (Google AdMob) - duenne Bruecke nach Java (android/AdManager.java). Auf allen anderen Plattformen (Desktop, iOS) sind die
// Aufrufe leer, der Rest des Spiels muss also nichts ueber Werbung wissen.
#include "axmol/platform/PlatformConfig.h"
#if AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
#    include "axmol/platform/android/jni/JniHelper.h"
#endif

namespace Ads
{
// Schalter: Werbung ganz aus = false (dann wird nichts angezeigt, auch auf Android nicht)
constexpr bool kEnabled          = true;
constexpr bool kBanner           = true;   // Banner unten im Menue
constexpr bool kInterstitial     = true;   // Vollbild-Anzeige zwischendurch
constexpr int  kLevelsPerInterstitial = 3; // nach jedem n-ten gewonnenen Level (nie im Editor-Testspiel)

inline void call(const char* method)
{
#if AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
    if (kEnabled) ax::JniHelper::callStaticVoidMethod("dev.axmol.app.AdManager", method);
#else
    (void)method;
#endif
}

inline void showBanner() { if (kBanner) call("showBanner"); }
inline void hideBanner() { if (kBanner) call("hideBanner"); }
inline void showInterstitial() { if (kInterstitial) call("showInterstitial"); }
}  // namespace Ads
