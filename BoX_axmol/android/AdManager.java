package dev.axmol.app;

import android.app.Activity;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;

import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;
import com.google.android.ump.ConsentInformation;
import com.google.android.ump.ConsentRequestParameters;
import com.google.android.ump.UserMessagingPlatform;

/**
 * Werbung fuer BoX (Google AdMob): Banner (im Menue) und Interstitial (nach jedem n-ten Level).
 * Die Methoden showBanner / hideBanner / showInterstitial werden per JNI aus C++ aufgerufen (siehe Source/Ads.h) und duerfen
 * von jedem Thread kommen - die Arbeit passiert auf dem UI-Thread.
 *
 * Es sind Googles TEST-Anzeigen-IDs eingetragen. Vor der Veroeffentlichung eigene IDs aus der AdMob-Konsole verwenden
 * (und die App-ID im AndroidManifest.xml ersetzen).
 */
public final class AdManager {
    private static final String TAG = "BoXAds";

    // Googles oeffentliche Test-IDs (zeigen Test-Anzeigen, kein Account noetig)
    private static final String BANNER_ID       = "ca-app-pub-3940256099942544/6300978111";
    private static final String INTERSTITIAL_ID = "ca-app-pub-3940256099942544/1033173712";

    private static Activity activity;
    private static boolean adsStarted = false;     // Einwilligung geklaert + MobileAds initialisiert
    private static boolean wantBanner = false;     // C++ wollte den Banner schon, bevor die Werbung bereit war
    private static AdView banner;
    private static InterstitialAd interstitial;
    private static boolean loadingInterstitial = false;

    private AdManager() {}

    /** Aus AppActivity.onCreate: erst die Einwilligung (DSGVO/UMP) einholen, dann die Werbung starten. */
    public static void init(final Activity a) {
        activity = a;
        a.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final ConsentInformation info = UserMessagingPlatform.getConsentInformation(activity);
                ConsentRequestParameters params = new ConsentRequestParameters.Builder().build();
                info.requestConsentInfoUpdate(activity, params,
                    new ConsentInformation.OnConsentInfoUpdateSuccessListener() {
                        @Override
                        public void onConsentInfoUpdateSuccess() {
                            UserMessagingPlatform.loadAndShowConsentFormIfRequired(activity,
                                new com.google.android.ump.ConsentForm.OnConsentFormDismissedListener() {
                                    @Override
                                    public void onConsentFormDismissed(com.google.android.ump.FormError formError) {
                                        if (formError != null) Log.w(TAG, "Consent form: " + formError.getMessage());
                                        if (info.canRequestAds()) startAds();
                                    }
                                });
                        }
                    },
                    new ConsentInformation.OnConsentInfoUpdateFailureListener() {
                        @Override
                        public void onConsentInfoUpdateFailure(com.google.android.ump.FormError formError) {
                            Log.w(TAG, "Consent info: " + formError.getMessage());
                            if (info.canRequestAds()) startAds();   // z.B. offline, aber frueher schon zugestimmt
                        }
                    });
                if (info.canRequestAds()) startAds();               // Einwilligung liegt schon vor
            }
        });
    }

    private static void startAds() {
        if (adsStarted || activity == null) return;
        adsStarted = true;
        MobileAds.initialize(activity, new com.google.android.gms.ads.initialization.OnInitializationCompleteListener() {
            @Override
            public void onInitializationComplete(com.google.android.gms.ads.initialization.InitializationStatus status) {
                Log.i(TAG, "AdMob initialisiert");
            }
        });
        loadInterstitial();
        if (wantBanner) showBanner();
    }

    // ---------------------------------------------------------------- Banner

    public static void showBanner() {
        wantBanner = true;
        if (activity == null) return;
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (!adsStarted) return;                             // startAds() holt es nach
                if (banner == null) {
                    banner = new AdView(activity);
                    banner.setAdUnitId(BANNER_ID);
                    banner.setAdSize(AdSize.BANNER);
                    FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                        Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
                    activity.addContentView(banner, lp);
                    banner.loadAd(new AdRequest.Builder().build());
                }
                banner.setVisibility(View.VISIBLE);
                banner.resume();
            }
        });
    }

    public static void hideBanner() {
        wantBanner = false;
        if (activity == null) return;
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (banner != null) {
                    banner.pause();
                    banner.setVisibility(View.GONE);
                }
            }
        });
    }

    // ---------------------------------------------------------------- Interstitial

    private static void loadInterstitial() {
        if (activity == null || loadingInterstitial || interstitial != null) return;
        loadingInterstitial = true;
        InterstitialAd.load(activity, INTERSTITIAL_ID, new AdRequest.Builder().build(),
            new InterstitialAdLoadCallback() {
                @Override
                public void onAdLoaded(InterstitialAd ad) {
                    interstitial = ad;
                    loadingInterstitial = false;
                    ad.setFullScreenContentCallback(new FullScreenContentCallback() {
                        @Override
                        public void onAdDismissedFullScreenContent() {
                            interstitial = null;
                            loadInterstitial();                      // naechste Anzeige vorladen
                        }

                        @Override
                        public void onAdFailedToShowFullScreenContent(com.google.android.gms.ads.AdError error) {
                            interstitial = null;
                            loadInterstitial();
                        }
                    });
                }

                @Override
                public void onAdFailedToLoad(LoadAdError error) {
                    Log.w(TAG, "Interstitial: " + error.getMessage());
                    interstitial = null;
                    loadingInterstitial = false;
                }
            });
    }

    /** Zeigt das Interstitial, falls eines geladen ist (sonst passiert nichts - das Spiel wird nie blockiert). */
    public static void showInterstitial() {
        if (activity == null) return;
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (interstitial != null) interstitial.show(activity);
                else loadInterstitial();
            }
        });
    }
}
