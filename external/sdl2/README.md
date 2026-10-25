# Automatic desktop SDK

`bootstrap.json` records the exact upstream SDL2 and ALSA Git revisions and the
SHA-256 hashes of the X.org header archives used by `tools/sdl2/ensure_sdk.py`.
SDL2 2.30.0 matches the version used by the earlier Android playback work.

A working installed SDL2 SDK is preferred. Otherwise the workspace downloads and
builds the pinned sources under its build directory, without system installation.
On Linux it can supply missing X11 and ALSA development headers while using the
platform's existing X11/ALSA runtime libraries. Only headers and licenses are
extracted from the X.org archives. Archives are bounded, verified and extracted
without following symbolic links or parent-directory paths.

The resulting SDL library is static; OS display/audio runtime libraries remain
platform dependencies. The SDK probe records compiled video/audio drivers and
licenses are included in installed player packages. A compiled driver or SDL dummy
test does not establish that a physical window/audio device worked on a machine.

Use `--no-player` for an explicit console-only build that needs no SDL bootstrap.
Upstream sources remain outside tracked source files in the build cache; this
folder contains provenance and the SDL license, not a full third-party checkout.
