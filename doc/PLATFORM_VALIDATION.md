# Platform validation runner — v0.361

[Русское руководство](PLATFORM_VALIDATION.ru.md)

Stage 9 requires actual builds **and execution** on each OS, as well as acceptance
of stages 1–8. The new tools make those runs repeatable; they do not supply missing
devices, SDKs or evidence. HEADLESS is the only implemented presentation adapter.
No source/history archives are created, in accordance with the standing delivery
instruction. All bundled dependencies remain static CMake targets.

## Native Windows, Linux and macOS

Use Python 3.8+, Git, CMake/CTest 3.26+ and the native C/C++ toolchain. From the
repository root:

```sh
python scripts/validate_platform.py --profile all --jobs 4
```

The runner uses `cmake/profiles/agg.defs`, `cairo.defs` and `minimal.defs`, without
editing DEFS.txt. AGG/Cairo enable fonts, codecs, SQLite, OS ports and render jobs;
minimal disables graphics/fonts/codecs/SQLite but retains real workers/networking.
All profiles build tests, examples and the optional reference runner in Release.
CTest retains assertions. Configure/build/test failures stop the run; zero tests
is an error. Existing cross-build directories are rejected before execution.
Profiles are sequential; `--jobs` controls build/test parallelism within each.

`--cmake`, `--ctest`, `--generator` and `--build-root` select installed tools and
output locations. On Windows a Visual Studio generator can be selected explicitly;
the runner passes `--config Release` to multi-configuration builds and CTest.
Run each OS separately; a Linux run does not prove Windows/macOS behavior.

Output defaults to ignored `build-platform/`: per-phase logs and
`platform-evidence.json` record the source commit, dirty flag, version, actual host,
compiler paths, CMake/CTest versions, commands, exit statuses and overall result. A failed or interrupted
run cannot produce an overall passing record. This is development evidence rather
than a release certificate; generated reports/logs must not be committed.

## Optional generated-instruction execution

With Unicorn installed in the selected Python environment, add:

```sh
python scripts/validate_platform.py --profile all --jit-emulator --jobs 4
```

This is an explicit additional gate. CTest's Release JSON supplies the configured
`flashvm_tests_jit_backends` command, including launch arguments; the runner does
not guess executable locations. It records Unicorn's reported version, generates
fixtures in a fresh directory per profile/run, then executes
`check_jit_backends.py`. Missing/duplicate/unavailable fixture tests, a missing
emulator, failed generation or failed emulation keep the profile/report failed.
No old fixture directory is reused. An optimized Python checker (`-O` or inherited
`PYTHONOPTIMIZE`) fails explicitly because its assertions are required evidence.

Per-profile `jit_encoder` evidence records scope, status, version and fixture path;
commands and complete phase logs remain in the ignored build directory. The
optional dependency is a validation tool, not a FlashVM runtime dependency or a
bundled library. Normal native validation does not require Unicorn. The new
`flashvm_platform_encoder_host` CTest runs five pure-Python orchestration tests
without importing Unicorn and is omitted from cross-build execution.

The current cases exercise synthetic helper addresses and generated instructions
for x86, ARM, ARM64, RV32 and RV64: ordinary/far branches, Thumb helper calls,
high 64-bit helper addresses and success/error exits. They do not run full FlashVM
or its GC on a foreign CPU, and cannot replace Windows/macOS/Android acceptance.

## Android device/emulator

Install an Android NDK, SDK platform-tools with shell-v2 ADB support and a device
or emulator with API 24+. Build for its ABI. Use the NDK's static C++ runtime so
that the adapter need not deploy libc++_shared.so. For example, with explicit local
paths and an already selected device serial (replace placeholders):

```sh
cmake -S . -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/ndk/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 \
  -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release \
  -DFVM_DEFS_FILE="$PWD/cmake/profiles/agg.defs" \
  '-DCMAKE_CROSSCOMPILING_EMULATOR=/absolute/path/to/python3;/absolute/path/to/FlashVM/scripts/android_test.py;--serial;DEVICE_SERIAL;--directory;/data/local/tmp/flashvm-SESSION;--timeout;175'
cmake --build build-android --parallel 4
ctest --test-dir build-android -C Release --output-on-failure --no-tests=error --parallel 2
```

Use a distinct SESSION directory per build/run and the same directory for all tests
in that run. Writer/reader CTest fixtures then share device files. The adapter
requires an explicit serial, pushes each target, enters that directory and executes
the target with quoted arguments. Staging/transport errors fail the test. ADB
shell-v2 returns the remote program's exit status; an `assert` failure cannot be
masked by a successful cleanup command. It never runs Android binaries on the host.
CTest's test timeout remains authoritative (normally 30 seconds; selected allocation
tests 180). On transport loss or timeout inspect the device for remaining processes
before another run. After the suite, remove only your SESSION directory on the device.
The adapter does not remove shared fixtures between tests.

Host Python multi-process/network/timezone/reference harnesses are explicitly omitted
from cross-build CTest registration: they spawn executables directly and cannot
automatically use `CMAKE_CROSSCOMPILING_EMULATOR`. Their native executable components
still run on the device; equivalent device-side orchestration remains an acceptance
gap. This omission is printed at configuration time. No Python runtime is required
on the device. Cross-compiling without an emulator remains build-only evidence.

Android uses the existing Linux-family JIT/memory paths where compiler/ABI selection
permits them. This is an implementation observation, not a device execution claim.
Windows/macOS JIT availability remains restricted by the existing backend/memory
selection; interpreter fallback does not prove a native backend for those OSes.

The adapter's host regression test checks error propagation, timeout and argument
quoting with a simulated transport. It is clearly named `flashvm_android_adapter_host`
and never counts as a real Android run.

## Acceptance limits

See [RELEASE_ACCEPTANCE.md](RELEASE_ACCEPTANCE.md) and [ROADMAP.md](ROADMAP.md).
No Windows/macOS/Android runtime acceptance is inferred from these scripts. No
stage is closed by adding a runner or by the number of passing Linux tests.

## v0.349 local evidence

Linux x86-64 GCC/G++ 13.3, CMake/CTest 4.4.4 Release: AGG 701/701, Cairo 701/701,
minimal 682/682 in final full CTest runs, including examples and the reference
runner. Initial/intermediate runs had BAD_COMMAND errors because some generated
ELFs lacked execute permission. Restoring executable permissions allowed their
focused checks and the final full runs to pass. The runner correctly recorded
the initial failures; it never changes permissions or retries failed tests itself.
No runtime behavior failure was retried away. The five simulated-transport checks
pass inside the host adapter test. Cross-config inspection checked 675 target
commands and fixture ordering; the native driver rejected that cross target.
Missing-tool and old-tool rejection were checked separately. These are Linux and
host/configuration checks only, not Android or other-OS execution.

### v0.361 execution evidence

The actual all-profile command with --jit-emulator passes 733 AGG, 733 Cairo and
714 minimal CTest checks and 26 Unicorn 2.1.4 cases in each profile. An initial
minimal run failed correctly on two ELF launch permissions; restoring generated
execute bits allowed the rerun to pass. Five orchestration tests and an explicit
optimized-checker rejection also pass. No foreign OS/runtime gate is closed.
