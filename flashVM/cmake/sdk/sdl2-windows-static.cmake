# Initial cache for a target-built static SDL 2.30 Windows SDK, separate from the VM sources.
# Pass to SDL's own configure with -C and the explicit Windows toolchain; never use a host SDK.
# Native Windows video and WASAPI/WinMM audio stay enabled, alongside deterministic dummy drivers.
set(SDL_SHARED OFF CACHE BOOL "No SDL2.dll dependency")
set(SDL_STATIC ON CACHE BOOL "Build target static SDL")
set(SDL_TEST OFF CACHE BOOL "No SDL test support library")
set(SDL_TESTS OFF CACHE BOOL "No target test executables during SDK acquisition")
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "FlashVM provides its own console/UTF-16 entry adapter")
set(SDL_AUDIO ON CACHE BOOL "Retain target audio services")
set(SDL_VIDEO ON CACHE BOOL "Retain target window services")
set(SDL_WASAPI ON CACHE BOOL "Native Windows audio")
# SDL 2.30 enables WinMM as part of desktop Windows SDL_AUDIO; there is no SDL_WMM option.
set(SDL_DUMMYAUDIO ON CACHE BOOL "Explicit dummy testing remains available")
set(SDL_DUMMYVIDEO ON CACHE BOOL "Explicit dummy testing remains available")
# Keep the SDK independent of external host libraries and optional GPU/input integrations.
# The player rasterizes with its statically linked AGG adapter and SDL software presentation.
foreach(FVM_SDL_OPTION IN ITEMS
    DIRECTX RENDER_D3D OPENGL OPENGLES VULKAN LIBSAMPLERATE DISKAUDIO
    HIDAPI JOYSTICK HAPTIC SENSOR)
  set(SDL_${FVM_SDL_OPTION} OFF CACHE BOOL "Disabled in the explicit Windows software SDK")
endforeach()
