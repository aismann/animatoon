#include "AppDelegate.h"

#include "Config.h"
#include "GameScene.h"
#include "Sound.h"
#include "axmol/audio/AudioEngine.h"

using namespace ax;

// The original game rendered at 512x512 (the texture atlases are 512x512) and scaled the window up.
static const ax::Size designResolutionSize = ax::Size(512, 512);

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate() {}

void AppDelegate::applicationWillLaunch()
{
    // settings.ini / chomp.cfg are read before the window exists (Vsync, Windowwidth, Windowed)
    Settings::get().load();

    ContextAttrs contextAttrs = {.powerPreference = PowerPreference::HighPerformance};
    contextAttrs.vsync        = Settings::get().vsync != 0;
#if AX_TARGET_PLATFORM != AX_PLATFORM_WIN32
    contextAttrs.renderScaleMode = RenderScaleMode::Physical;
#endif
    setContextAttrs(contextAttrs);
}

bool AppDelegate::applicationDidFinishLaunching()
{
    auto director   = Director::getInstance();
    auto renderView = director->getRenderView();
    auto& st        = Settings::get();
    if (!renderView)
    {
#if (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32) || (AX_TARGET_PLATFORM == AX_PLATFORM_MAC) || \
    (AX_TARGET_PLATFORM == AX_PLATFORM_LINUX)
        if (st.windowed)
            renderView = RenderView::createWithRect("Chompston", ax::Rect(0, 0, (float)st.windowWidth, (float)st.windowWidth));
        else
            renderView = RenderView::createWithFullscreen("Chompston");
#else
        renderView = RenderView::create("Chompston");
#endif
        director->setRenderView(renderView);
    }

    director->setStatsDisplay(false);
    director->setAnimationInterval(1.0f / 60);
    director->setClearColor(Color(0.f, 0.f, 0.f, 1.f));
    renderView->setDesignResolutionSize(designResolutionSize.width, designResolutionSize.height,
                                        ResolutionPolicy::SHOW_ALL);

    auto scene = utils::createInstance<GameScene>();
    director->runWithScene(scene);
    return true;
}

void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->deactivate();
    AudioEngine::pauseAll();
}

void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->activate();
    AudioEngine::resumeAll();
}

void AppDelegate::applicationWillQuit()
{
    Settings::get().saveUserData();
    Sound::shutdown();
}
