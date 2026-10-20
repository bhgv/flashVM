# Platform validation runner — v0.633

[Русское руководство](PLATFORM_VALIDATION.ru.md)

Stage 9 requires actual builds **and execution** on each OS, as well as acceptance
of stages 1–8. The new tools make those runs repeatable; they do not supply missing
devices, SDKs or evidence. HEADLESS is the only implemented presentation adapter.
No source/history archives are created, in accordance with the standing delivery
instruction. All bundled dependencies remain static CMake targets.

## Native Windows, Linux and macOS

Use Python 3.8+, Git, CMake/CTest 3.26+ and the native C/C++ toolchain. From the
component root:

```sh
python scripts/validate_platform.py --profile all --jobs 4
```

The runner uses `cmake/profiles/agg.defs`, `cairo.defs`, `minimal.defs` and
`minimal-jit.defs`, without
editing DEFS.txt. AGG/Cairo enable fonts, codecs, SQLite, OS ports and render jobs;
The minimal profiles disable graphics/fonts/codecs/SQLite but retain real workers/networking;
minimal-jit selects automatic JIT for new contexts.
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

## Compiler provenance — v0.496

Each profile now records `toolchains.C` and `toolchains.CXX` from CMake's active
`CMakeFiles/<cache-version>/CMake{C,CXX}Compiler.cmake` detection results: compiler
ID/version, pointer size in bytes, platform, architecture, frontend, target and
simulation identity when supplied. The exact metadata path is retained. Compiler
paths and generator remain in the existing `compiler` record. Missing optional
fields remain empty; they are not inferred from the Python host architecture.

The runner reads literal scalar detection values without evaluating generated
CMake code. Missing language metadata, incomplete vendor/version/ABI evidence,
malformed active-cache version and mismatched C/C++ pointer sizes fail validation
before building. Old metadata directories are ignored in favor of the active
cache version. This records configured toolchain identity, not a new foreign
runtime run or proof that all code paths use the selected instruction set.
Ten host orchestration tests cover encoder evidence and compiler provenance,
including independent compiler vendors and rejection of incomplete or mixed ABI
metadata. Synthetic metadata tests do not establish actual MSVC execution.

### v0.496 Linux Cairo execution evidence

A clean bundled Cairo/fonts/codecs/SQLite/OS-ports build completed on Linux x86-64
with GNU C/C++ 13.3.0 and CMake/CTest 4.4.4. The final full runner passes 1013/1013
checks, including the reference runner, and 26 Unicorn 2.1.4 synthetic ISA/ABI
cases for x86/ARM/ARM64/RV32/RV64. x64 executes natively in the JIT tests.
Sixteen initial generated binaries lacked execute permission; after restoring
those bits, the entire runner passed rather than merely its failed subset.
The passing development record identifies v0.496 with dirty=true on parent
12aa646, including actual compiler IDs, versions and 8-byte C/C++ pointer sizes.

The sampled linked Array test has only system C/C++ runtime NEEDED libraries;
Cairo/fonts/codecs/SQLite remain statically linked. Release test flags include
-std=c++98, -pedantic-errors and -UNDEBUG. Ten host orchestration tests pass.
This is Linux evidence; Windows/macOS/Android and overall release acceptance
remain open. No source/history archives were created.

### Direct Linux ELF dependencies — v0.511

Add `--elf-dependencies` (optionally `--readelf /path/to/readelf`) to the native
runner to require a GNU readelf audit after CTest. It discovers executable/shared
ELF arguments from the authoritative CTest JSON inside the build directory,
deduplicates paths and records every artifact's direct DT_NEEDED entries and the
actual GNU readelf version. ELF phases run with LC_ALL=C, recorded in command
evidence. Static executables without a dynamic section are accepted.

The audit rejects direct shared linkage of bundled FlashVM/AGG/Cairo/Freetype/
FriBidi/GIF/HarfBuzz/JPEG/PNG/Unibreak/Pixman/SQLite/zlib families. System C/C++
runtime dependencies remain allowed and visible. Missing/malformed discovery,
missing dynamic-section evidence, forbidden libraries or tool failures prevent a
passed record; a stale pass is replaced before execution. Non-Linux hosts reject
this optional audit. Eight Python host tests cover discovery/header/path handling,
soname policy, malformed output, phase failures and report state.

This is direct-link evidence for discovered CTest ELF arguments. It does not
inspect transitive dependencies, dlopen, libraries hidden by wrapper scripts or
all possible shipped files. It does not certify other OSes, presentation, SWF
compatibility or completion of roadmap stage 9.

Validation v0.511: the native Linux x86-64 Cairo runner passed all 1067 CTest
cases, 26 Unicorn ISA/ABI branch/error cases and the new audit of 1056 ELF files.
GNU readelf 2.42 reported only libc.so.6, libgcc_s.so.1, libm.so.6 and
libstdc++.so.6 as direct dependencies. After restoring execute permission on seven
generated test binaries, the entire runner was repeated successfully. The report
honestly records development version 0.511, parent 56bd1f8 and dirty=true.
Minimal/AGG passed four targeted configuration/version/host checks each; Python
suites passed 8 ELF-audit and 10 existing runner tests. These are Linux results;
other OS acceptance, anonymous Undefined-this semantics and roadmap stages remain open.

v0.513 checkpoint: after qualified-this runtime changes, the native Linux x64
Cairo suite passed 1074 cases (permission repairs/reruns included). A fresh
GNU readelf audit passed for 1063 CTest ELF arguments; only libc.so.6,
libgcc_s.so.1, libm.so.6 and libstdc++.so.6 appeared in direct DT_NEEDED.
No new foreign-OS or transitive/dlopen acceptance is implied.

v0.514 extends the bundled-library DT_NEEDED policy to upstream NanoSVG sonames
(libnanosvg.so and versioned names, including absolute paths). The fvm_* spelling
was already covered; upstream spelling was missing despite the static bundled
NanoSVG target. Regression cases also verify that this failure replaces stale
passed audit evidence. Linux-only direct-dependency audit scope is unchanged.
Validation: eight ELF and ten platform-encoder Python cases passed; four targeted
configuration/version/host checks passed per minimal, AGG and Cairo profile.
The revised policy passed again on 1063 Cairo ELF artifacts from the v0.513
runtime baseline (plus refreshed build-config version), with the same four
system sonames. No C++ runtime logic changed in v0.514.

### v0.519: missing executable discovery

The ELF audit now rejects an absolute CTest command inside the build tree when
it is missing or is a directory. Another valid ELF no longer masks the missing
command or leaves a stale passing audit status. Ordinary output arguments may
still be absent before execution. External launchers, relative commands and
non-ELF scripts are not certified as ELF artifacts by this audit; successful
full CTest execution remains a separate required phase.

Regression coverage: test_elf_dependencies reproduces three false passes on
v0.518, including stale successful evidence, then checks the corrected policy.
This improves stage 9 evidence; it does not complete platform acceptance.

Validation: nine ELF audit unit cases and ten encoder/host cases passed; all
four configuration/version/audit-host checks passed in minimal, AGG and Cairo.
Fresh GNU readelf audit passed for 1076 current Cairo ELF artifacts, with only
libc, libgcc_s, libm and libstdc++ in direct DT_NEEDED. Runtime code is unchanged
from the full v0.518 suites (1067/1086/1087); those suites are not claimed as new
v0.519 full runs. Actual foreign-OS and transitive/dlopen acceptance remain open.

## Parallel Android staging — v0.526

Each ADB adapter invocation now uploads a uniquely named executable. Parallel
CTest entries can invoke the same build target without overwriting an executable
that another test is using. All invocations retain the configured shared working
directory, so file-process fixtures keep their existing paths and ordering.
Only the invocation's own binary is removed after execution or a failed push;
fixture data is preserved. A cleanup failure cannot turn a passing target into a
pass, and cannot replace an already nonzero target exit code. Execution/staging
exceptions remain primary if best-effort cleanup also fails. A hard host kill or
unavailable device can still leave a uniquely named binary for manual cleanup.

Nine host transport tests include overlapping executions of the same target,
argument quoting, partial push cleanup, execution timeout, cleanup failures and
shared fixture paths. These are mocked ADB tests, not an Android device run.

## Recorded Linux matrix — v0.530

The v0.529 implementation passed minimal (1095), AGG (1115) and Cairo (1115),
plus fresh Unicorn/direct ELF gates. See [checkpoint](LINUX_CHECKPOINT_V529.md)
for exact source/tool provenance, permission repairs and acceptance limits.
This documentation checkpoint adds no runtime behavior or foreign-OS evidence.


### Automatic-JIT release profile / профиль автоматического JIT

`--profile minimal-jit` selects `cmake/profiles/minimal-jit.defs`; `--profile all`
now also includes this fourth profile. It uses the same CTest, fresh optional
Unicorn and direct ELF audit gates as other profiles. This checks the automatic
context default across the full suite, including interpreter fallback, rather
than only tests that explicitly enable JIT. See [RU/EN contract](JIT_BUILD_DEFAULT.md).

Профиль `minimal-jit` проходит те же проверки, что остальные; `all` включает все
четыре профиля. Проверяется начальный автоматический режим контекстов во всём
наборе тестов. Это Linux-проверка; чужие ОС/CPU она не подтверждает.


### v0.532 — automatic-JIT Linux validation checkpoint

Recorded / зафиксировано: clean v0.531 minimal-jit release-runner validation,
1097 CTest cases, 26 fresh Unicorn cases and 1086 direct ELF artifacts;
precommit minimal/JIT 1096 each, 30 definition cases and three ASan/UBSan cases.
See [RU/EN evidence and limits](LINUX_JIT_CHECKPOINT_V531.md). Documentation
only; real foreign OS/CPU acceptance and all nine stages remain open.

## Source stability (v0.633)

Before tools run and again before reporting success, the runner fingerprints the
component's Git-listed inputs. The report contains `source_start` and `source_end`
with HEAD, SHA-256 and file count. A mismatch or unreadable endpoint fails the
whole run even when all individual profiles passed. Failed tool phases can stop
before the final snapshot; absence of `source_end` is not stability evidence.

The digest includes tracked and nonignored untracked paths, working file bytes,
owner executable bits, missing tracked files, symlink target names, and Git index
entries. Length-prefixed records distinguish paths and binary contents. Already
dirty workspaces are supported: changing dirty content is detected even when the
dirty flag remains true. Only this independent component is scanned; sibling
projects and ignored build products are excluded. Keep `--build-root` outside the
component or under an ignored build directory to avoid fingerprinting generated
logs as source inputs. Submodule directories and other special input types fail
closed; regular source distributions need no submodules.

This is a comparison of two endpoints, not an immutable checkout or a filesystem
lock: edit-and-restore during execution can escape detection. External symlink
target contents, ignored SDK/dependency installations, tool binaries and environment
changes are not hashed. Record these separately for release acceptance; a matching
digest alone does not prove reproducibility, an original-Player comparison, device
acceptance or completion of any roadmap stage. The report schema remains 1 with
additive source fields; consumers must still inspect the overall `status`.

Run the isolated host regression (real temporary Git repositories, simulated
build tools; no claim of native VM execution):

```sh
python3 scripts/test_platform_source.py
```

Schema 2 and v0.641–0.645 tooling: [release evidence](RELEASE_EVIDENCE.md). Historical schema-1 notes are not new acceptance evidence.
