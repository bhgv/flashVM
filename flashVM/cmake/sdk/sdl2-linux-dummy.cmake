# Initial cache for a target-built, static SDL 2.30 SDK with dummy audio/video.
# Pass this file to SDL's own CMake configure with -C, alongside an explicit toolchain.
# This is a build/link validation SDK: it cannot open physical windows or audio devices.
# Keep the source distribution and upstream license outside the FlashVM repository.
set(SDL_SHARED OFF CACHE BOOL "Do not build a shared SDL runtime")
set(SDL_STATIC ON CACHE BOOL "Build the static target SDL archive")
set(SDL_TEST OFF CACHE BOOL "Do not build SDL's test library")
set(SDL_TESTS OFF CACHE BOOL "Do not build SDL's executable tests")
set(SDL_DUMMYAUDIO ON CACHE BOOL "Enable deterministic dummy audio")
set(SDL_DUMMYVIDEO ON CACHE BOOL "Enable deterministic dummy video")
set(SDL_PTHREADS ON CACHE BOOL "Use target pthreads")
# Disable drivers/dependencies that could accidentally come from the build host.
# This preserves software rendering and SDL threads/timers/event processing.
foreach(FVM_SDL_OPTION IN ITEMS
    ALSA JACK ESD PIPEWIRE PULSEAUDIO ARTS NAS SNDIO OSS LIBSAMPLERATE
    DBUS IBUS X11 WAYLAND RPI KMSDRM VIVANTE OFFSCREEN OPENGL OPENGLES
    VULKAN HIDAPI LIBUDEV JOYSTICK HAPTIC SENSOR SYSTEM_ICONV)
  set(SDL_${FVM_SDL_OPTION} OFF CACHE BOOL "Disabled in the explicit dummy SDK")
endforeach()
