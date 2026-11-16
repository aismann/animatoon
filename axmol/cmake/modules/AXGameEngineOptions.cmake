# Animatoon only needs the 2D core + Dear ImGui, everything else is switched off
# to keep the build small and fast.
set(AX_EXT_HINT OFF CACHE BOOL "The default extensions hint" FORCE)
set(AX_ENABLE_EXT_IMGUI ON CACHE BOOL "Build extension ImGui" FORCE)
set(AX_ENABLE_EXT_INSPECTOR OFF CACHE BOOL "Enable extension Inspector" FORCE)
set(AX_ENABLE_EXT_SDFGEN OFF CACHE BOOL "Build extension SDFGen" FORCE)

set(AX_ENABLE_3D OFF CACHE BOOL "Build 3D support" FORCE)
set(AX_ENABLE_PHYSICS_2D OFF CACHE BOOL "Build Physics2D support" FORCE)
set(AX_ENABLE_AUDIO OFF CACHE BOOL "Build audio support" FORCE)
set(AX_ENABLE_VIDEO OFF CACHE BOOL "Build video support" FORCE)
set(AX_ENABLE_MSEDGE_WEBVIEW2 OFF CACHE BOOL "Build with msedge webview2 support" FORCE)
set(AX_ENABLE_MFMEDIA OFF CACHE BOOL "Build with mfmedia support" FORCE)
