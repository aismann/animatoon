#include "GameHost.h"

#include <thread>

#include "game/Game.h"

namespace
{
std::thread g_thread;
}

namespace GameHost
{

void start(const char* resourceRoot, const char* writableRoot)
{
    if (g_thread.joinable())
        return;
    auto& p = pb::Platform::get();
    p.setResourceRoot(resourceRoot);
    p.setWritableRoot(writableRoot);
    g_thread = std::thread(
        []
        {
            try
            {
                ll::game_main();
            }
            catch (const pb::QuitException&)
            {
            }
            catch (const std::exception& e)
            {
                fprintf(stderr, "[Lost Labyrinth] fatal: %s\n", e.what());
            }
            pb::Platform::get().requestQuit();
        });
}

void stop()
{
    pb::Platform::get().requestQuit();
    if (g_thread.joinable())
        g_thread.join();
}

}  // namespace GameHost
