// Owns the game thread that runs the (blocking) port of the original main loop.
#pragma once

namespace GameHost
{
void start(const char* resourceRoot, const char* writableRoot);
void stop();
}  // namespace GameHost
