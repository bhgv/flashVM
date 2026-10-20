# Player build metadata

Since v0.669 `flashvm_player --version` reports VERSION.txt, the linked VM ABI,
target pointer width, JIT backend availability, whether the SDL audio bridge was
compiled, and header/linked SDL versions. It opens no file, window or audio device;
a nonexistent video driver does not prevent the query. Process locale is unchanged.
These are build metadata, not device or compatibility acceptance.

Development version and ABI are independent. Pointer width does not identify a
processor model. JIT availability does not guarantee every program is supported
or later executable-memory allocation succeeds. Compiled audio support guarantees
neither a driver, decoder, endpoint nor streaming-sound playback.

For C hosts, `fvm_desktop_build_info` fills `FVM_DesktopBuildInfo` without allocation;
null out returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). The version string is static and must not
be freed. This is an internal host interface in frontend/common/player.h, not a
new stable VM library ABI. Its extern C guards support C++98; a separate test
actually links a C++ host to the C89 player and invokes both interfaces with
nonexistent SDL device drivers.

CMake generates desktop_build_version.h in the build directory. VERSION.txt changes
refresh it on the next build. Do not copy generated headers into the source tree
or reuse one from another target build. To compile player.c into a custom host,
provide the matching frontend build's generated include directory along with the
usual FlashVM and SDL headers.

Like `--help`, `--version` ends parsing: an earlier error is retained and subsequent
valid arguments are unused. Every argv pointer is validated first. After `--`,
the string `--version` is a filename.

```sh
./build-player/frontend/flashvm_player --version
cmake --build build-player --target flashvm_frontend_build_info
ctest --test-dir build-player -R frontend_build_info --output-on-failure
```

[Host interface](../frontend/common/player.h) ·
[Implementation](../frontend/common/player.c) ·
[C++98 linkage test](../frontend/tests/build_info.cpp) ·
[Native/cross build guide](DESKTOP_FRONTEND.md) ·
[Arguments and GC](PLAYER_EVENT_GC.md) ·
[Roadmap status](ROADMAP_STATUS.md)
## v0.669 validation

On Linux x86-64, 62 selected tests passed: 47 frontend/player/capture checks and
15 related VM scene/sound checks. This is not the complete project test suite.
The player uses SDL 2.30.12 dummy video and software rendering with the SDL audio
bridge disabled. Metadata was checked with nonexistent drivers. Actual object
reclamation was checked in interpreter and JIT; restoring the old timer-only
safe-point condition fails both regressions.

The full player also linked with target-specific static SDL for ARM64 Linux
(musl), RISC-V64 Linux (musl) and x86-64 Windows (GNU). ELF/PE types and machine
codes 183, 243 and 0x8664 were verified. These builds used the existing host-adapted
Zig 0.13, not an unmodified SDK acceptance run. Foreign binaries were not executed.
No new macOS/Android, physical window/audio or endurance acceptance is claimed.
[Structured C89/C++98 host / Встраивание через структуру](PLAYER_CONFIGURATION.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
