# DEFS.txt build selection

The top-level `DEFS.txt` is the authoritative build-key file. Uncomment exactly
one line per group; leave alternatives prefixed with `#`. Inline comments are
allowed. These are build keys, not runtime ActionScript settings. Configuration
rejects unknown keys, duplicates, empty groups and competing selections.

| Group | Default | Alternatives |
| --- | --- | --- |
| Rasterizer | GRAPHICS_AGG | GRAPHICS_CAIRO, NO_GRAPHICS |
| Target OS | AUTO_PLATFORM | WINDOWS, LINUX, MAC, ANDROID |
| Presentation | HEADLESS | FB, GLES, OGL, DX, SDL1, SDL2, SDL3 — not implemented |
| Image libraries | IMAGE_CODECS | NO_IMAGE_CODECS |
| Database | SQLITE | NO_SQLITE |
| Native input/TCP | OS_PORTS | NO_OS_PORTS |
| Worker render jobs | RENDER_JOBS | NO_RENDER_JOBS |
| Execution | INTERPRETER | JIT — not implemented |
| Media | NO_MEDIA | MEDIA — not implemented |

`AUTO_PLATFORM` resolves to the target OS reported by the CMake toolchain.
An explicit OS key must match that target: changing LINUX to WINDOWS does not
turn a Linux compiler into a Windows cross compiler. Android is selected before
the general Linux case. The selected adapter files come from `ports/<platform>/`.

`HEADLESS` keeps graphics in a host-owned RGBA8 image. It creates no native window
or screen connection; presentation belongs to the embedding application. Screen
adapter keys are reserved and fail explicitly when selected. Cairo's own GL or
Win32 surface code is not a FlashVM screen adapter and is not enabled here.

With `GRAPHICS_CAIRO`, CMake compiles bundled Cairo and Pixman and the FlashVM
raster API uses Cairo. With `GRAPHICS_AGG`, it compiles bundled AGG. `NO_GRAPHICS`
builds no FlashVM raster backend and returns FVM_UNSUPPORTED from valid raster calls.
IMAGE_CODECS independently builds PNG/GIF/JPEG and Cairo/NanoSVG for SVG.
See [image APIs and limits](IMAGE_CODECS.md).
zlib and mini_regex remain enabled in all profiles. `SQLITE` compiles the bundled
amalgamation, C adapter and SQLite programs; no system lookup or fallback occurs.

Configure a fresh build after selecting keys:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

For independent builds, copy the file, edit the copy and pass
`-DFVM_DEFS_FILE=/absolute/path/to/my-defs.txt`. Relative file paths are interpreted
by CMake's FILEPATH cache handling; absolute paths avoid working-directory
ambiguity. A changed DEFS.txt automatically triggers CMake reconfiguration on
the next build. `generated/flashvm_build_config.h` is generated in the build
directory; selected keys are also public compiler definitions on the flashvm target.

The default file has one enabled key in each group. The generated header contains
AUTO_PLATFORM and exactly one resolved OS macro. Old FVM_ENABLE_AGG,
FVM_ENABLE_CAIRO, FVM_ENABLE_SQLITE, FVM_ENABLE_OS_PORTS and
FVM_ENABLE_RENDER_JOBS and FVM_ENABLE_IMAGE_CODECS cache switches are accepted only if they agree with the
file. A conflicting old cache must be removed or replaced with a fresh build.
FVM_BUILD_TESTS/FVM_BUILD_EXAMPLES remain independent CMake options;
FVM_ENABLE_RENDER can additionally require an available rasterizer.

Linux without CMake: `python3 scripts/check.py` reads the same definitions and
compiles the same selected vendor lists. Use `--defs /path/to/file` or `--sanitize`.
The standalone runner resolves only Linux; cross-platform builds use CMake.
`python3 scripts/check_definitions.py` checks configuration rejection and
regeneration rules. `--cmake /path/to/cmake` selects a non-PATH installation.
`python3 scripts/check_vendor_sources.py` verifies the bundled source manifest.

The keys do not declare a complete SWF display engine, implemented JIT, or
validated Windows/macOS/Android runtime. Cairo's premultiplied intermediate
buffer can round low-alpha RGB differently from AGG; padding and untouched
pixels remain unchanged. See [checkpoint evidence](DEPENDENCY_CHECKPOINT.md).
