# Native and cross builds

Root builds now use selected owned sources in tools/; detached SDK overrides remain.
[First steps / Первые шаги](README-FIRSTSTEPS.md) · [Source manifests](tools/UPSTREAMS.md).

[Русский](BUILDING.ru.md). A successful cross build is compilation/link evidence,
not execution on that CPU/OS. Final [release gates](flashVM/doc/ROADMAP.md) remain
open until native/device and compatibility checks are complete.

## Independent components and language modes

| Component | CMake source directory | Main targets | Language |
| --- | --- | --- | --- |
| FlashVM | `flashVM` | `flashvm`, embedding examples | C89 ABI, C++98 implementation |
| Syntax frontends | `compiler` or `compiler/as1`, `as2`, `as3`, `limbo` | `as1_parse`, `as2_parse`, `as3_parse`, `limbo_parse`, namespaced libraries | C99 |
| FIR compiler/source adapter | `fir/v1` or `fir/v1/compiler` | `fir1`, `fir1_source`, `fir1_ir`, `fir1_source_api` | C99 |
| FIR optimizer | `fir/v1` or `fir/v1/optimiser` | `fir1_opt`, shared FIR C API | C99 |
| SWO/SWA/resource linker | `linker` | `swl`, `swl_core` | C99 |
| Combined C embedding tutorial | `linker/examples/ide` | `ide_pipeline` | C99 |

Each project owns its CMake configuration and explicit dependencies. The optional
[toolchain template](toolchains/explicit.cmake) can be copied with any detached
component; no component includes a sibling project's private implementation.
Keep the root MIT LICENSE and upstream notices in detached distributions.

**Host** runs CMake, Python, Coco/R and optional formatters. **Target** runs the
compiled VM/tool/IDE executable. Coco/R must be a host executable even when the
parser library is built for another CPU or OS. Pass its absolute path explicitly;
do not cross-compile Coco/R and attempt to run that target executable as a build
step. [External generator](https://github.com/bhgv/coco-r-C), not vendored here.

Building fir1 for ARM does not turn FIR into ARM machine code: FIR still emits
portable AVM1 SWO objects, and SWL produces SWF8. The tools' executable architecture
and their SWF bytecode output format are different choices. An LLVM/native FIR
backend is not implemented.

## Build and test on the current machine

Requirements: native C/C++ compilers, CMake 3.16+ for C99 components, Python 3.9+
for tests and an explicitly supplied host Coco/R. The VM accepts CMake 3.10+;
its native release reporter requires CMake/CTest 3.26+. Examples below use a
single-configuration generator. With Visual Studio/Xcode use `--config Release`
and `ctest -C Release`; executables may be inside `Release/` and end in `.exe`.

From repository root:

```sh
COCO_HOST=/absolute/path/to/coco-r-C
COCO_EXE="$COCO_HOST/sources/cocor"
cmake -S compiler -B build/native-compiler \
  -DCOCOR_ROOT="$COCO_HOST" -DCOCOR_EXECUTABLE="$COCO_EXE"
cmake --build build/native-compiler --parallel 2
ctest --test-dir build/native-compiler --output-on-failure

cmake -S fir/v1 -B build/native-fir \
  -DCOCOR_ROOT="$COCO_HOST" -DCOCOR_EXECUTABLE="$COCO_EXE"
cmake --build build/native-fir --parallel 2
ctest --test-dir build/native-fir --output-on-failure

cmake -S linker -B build/native-linker
cmake --build build/native-linker --parallel 2
ctest --test-dir build/native-linker --output-on-failure

cmake -S flashVM -B build/native-vm -DCMAKE_BUILD_TYPE=Release \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build build/native-vm --parallel 2
ctest --test-dir build/native-vm --output-on-failure
```

The minimal VM profile has no rasterizer, fonts, image codecs or SQLite; it keeps
native ports and render-job infrastructure. Use agg.defs or cairo.defs for bundled
static graphics dependencies, then separately validate their runtime behavior.
C99-only projects do not require the VM. Set TMPDIR to a writable directory if
the host has no usable standard temporary directory.

## Another Linux processor

Supply a complete target SDK, including compiler, assembler/linker, target libc
and C++ runtime for the VM. Different OS/CPU/ABI/compiler selections require
fresh build directories; editing a compiler in an existing CMake cache is unsafe.
The optional template preserves SDK variables in compiler probes, keeps real
executable link checks enabled and never finds host tools inside the sysroot.

```sh
cmake -S fir/v1 -B build/linux-arm64-fir \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/toolchains/explicit.cmake" \
  -DFVM_TARGET_SYSTEM=Linux -DFVM_TARGET_PROCESSOR=aarch64 \
  -DFVM_TARGET_CC=/sdk/bin/aarch64-linux-gnu-gcc \
  -DFVM_TARGET_AR=/sdk/bin/aarch64-linux-gnu-ar \
  -DFVM_TARGET_RANLIB=/sdk/bin/aarch64-linux-gnu-ranlib \
  -DCOCOR_ROOT="$COCO_HOST" -DCOCOR_EXECUTABLE="$COCO_EXE" \
  -DBUILD_TESTING=OFF
cmake --build build/linux-arm64-fir --parallel 2
```

For all syntax frontends change `-S` to compiler; for the linker use linker and
omit Coco/R settings. For VM use `-S flashVM`, add
`-DFVM_TARGET_CXX=/sdk/bin/aarch64-linux-gnu-g++`, the explicit minimal.defs path,
`-DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=OFF`. Optional FVM_TARGET_SYSROOT is
for an SDK that requires a separate sysroot; do not point it at host `/usr`.
Use matching riscv64-linux-gnu tools and processor riscv64 for RISC-V64, or the
SDK's corresponding x86/ARM32 tools and ABI. ISA baseline, float ABI and target
OS minimum must match the deployment machine and SDK; a CPU name alone is not
an ABI specification. The template expects a compiler/wrapper that already
selects its target; it does not invent compiler-specific `-march` options.

## Windows from another OS

Use a Windows SDK/MinGW toolchain with matching C and C++ runtimes. The existing
[MinGW x64 VM toolchain](flashVM/cmake/toolchains/mingw-x64.cmake) remains usable;
the explicit template also works for every component:

```sh
cmake -S linker -B build/windows-x64-linker \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/toolchains/explicit.cmake" \
  -DFVM_TARGET_SYSTEM=Windows -DFVM_TARGET_PROCESSOR=AMD64 \
  -DFVM_TARGET_CC=/sdk/bin/x86_64-w64-mingw32-gcc \
  -DFVM_TARGET_CXX=/sdk/bin/x86_64-w64-mingw32-g++ \
  -DFVM_TARGET_RC=/sdk/bin/x86_64-w64-mingw32-windres \
  -DBUILD_TESTING=OFF
cmake --build build/windows-x64-linker --parallel 2
```

Compiler/FIR substitutions and host Coco/R are as above. A Windows executable
built on Linux is not a Linux executable; Wine or an emulator is separate test
evidence, not native Windows acceptance. Audit actual DLL dependencies rather
than assuming a static archive also makes its platform runtime static.

## Android: different OS and CPU with the NDK

Use the NDK's own toolchain and target API/ABI rather than the generic template.
Coco/R, Python and CMake remain host tools. Example paths are SDK placeholders:

```sh
cmake -S fir/v1 -B build/android-arm64-fir \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DCOCOR_ROOT="$COCO_HOST" -DCOCOR_EXECUTABLE="$COCO_EXE" \
  -DBUILD_TESTING=OFF
cmake --build build/android-arm64-fir --parallel 2
```

Substitute compiler/linker sources as above; for VM supply minimal.defs and its
FVM_BUILD_TESTS/FVM_BUILD_EXAMPLES switches. Other NDK ABIs require separate build
directories. A standalone NDK CLI needs deployment to a suitable executable
location; an APK needs a native host/activity and packaging, not just a static
library. Existing [Android frontend](flashVM/frontend/android/README.md) and
[device adapter](flashVM/scripts/android_test.py) document the separate player/
CTest device integration. No new Android SDK/device run is claimed in this series.

On Android/Termux, native CMake/Clang builds use the native commands, with host
Coco/R built for that Android CPU. Desktop Coco/R is not executable on a phone.
Storage/downloads is useful for patches; build and execute inside Termux's app
filesystem. This guide does not claim every SDK dependency is available as a
particular Termux package.

## macOS and Apple CPU selection

On macOS use an installed Xcode/Command Line Tools SDK and native commands.
For a different supported Apple CPU pass `-DCMAKE_OSX_ARCHITECTURES=arm64` or
x86_64 in a fresh directory; universal builds may use `arm64;x86_64` if all
libraries/SDK slices support both. The build host still runs its own Coco/R.
Signing, JIT entitlements/MAP_JIT and runtime tests are independent requirements.
Cross-compilation from Linux needs a legally supplied Apple SDK and a compatible
Darwin linker/toolchain; this repository does not bundle or fabricate that SDK.
No new macOS native/cross-build acceptance is claimed here.

## Running tests after cross compilation

`BUILD_TESTING=OFF` applies to C99 projects; VM uses FVM_BUILD_TESTS=OFF. These
switches are deliberate: Python acceptance scripts spawn tool executables and
must not try to run a foreign binary on the host. Do not count disabled tests as
passes. CTest automatically adds CMAKE_CROSSCOMPILING_EMULATOR to direct executable
target tests, but not to arbitrary binaries spawned inside Python scripts.
For a full native tool suite, build/run on the target with Python/CMake/Coco/R;
for VM device/SDK protocols use its documented explicit adapters. Do not simply
copy a CTest build tree with absolute host paths and call it native acceptance.

Inspect artifacts (`readelf -h` for ELF, PE tools for Windows), link a real client
and then run it on the intended platform. Native release reporting deliberately
rejects cross builds: [release evidence](flashVM/doc/RELEASE_EVIDENCE.md).
Current build evidence and SDK limitations: [toolchains/VALIDATION.md](toolchains/VALIDATION.md).

Source/example navigation: [compiler catalog](compiler/examples.md),
[FIR examples](fir/v1/examples/README.md), [linker tutorial](linker/examples/ide/README.md),
[VM examples](flashVM/examples/README.md), [ATG style](tools/README.md).

## C99 representation contract

Frontends and SWL require eight-bit char and at least 32-bit int: UTF-8, the
2 MiB input bound and octet wire formats cannot silently become wide characters.
FIR additionally requires an eight-byte binary64 double (radix 2, 53 mantissa
bits, maximum exponent 1024). Known mixed float/integer word order is rejected
when the compiler exposes both macros. This is not a substitute for checking
IEEE representation on an unknown exotic target. Incompatible public headers
fail at compile time, before SWO payload copying can corrupt memory/output.
Five [header test groups](tools/test_portable_headers.py) execute 17 compile-only
GCC/Clang host probes: a combined normal C99 include and deliberate incompatible
limit/float macro fixtures. These are conditional-policy fixtures, not actual
16-bit CPU executions.

```sh
python3 tools/test_portable_headers.py --cc /host/path/cc
```

The template explicitly prevents host C++ fallback: enabling C++ without
FVM_TARGET_CXX stops configuration with FVM_TARGET_CXX_REQUIRED. C99-only
components may omit it. Four actual configure probes test this contract:
[test_toolchain_contract.py](tools/test_toolchain_contract.py).

## Structural FIR parser and five C APIs

Standalone FIR parser: `cmake -S fir/v1/frontend -B build-fir-syntax`, with the same
explicit COCOR_ROOT/COCOR_EXECUTABLE options. Targets: firsyn_parse and firsyn_frontend.
The full fir/v1 enables `fir1 syntax` by default; FIR_BUILD_SYNTAX_FRONTEND=OFF
disables it. To link all five APIs from compiler, explicitly pass
FIR_SYNTAX_ROOT=/absolute/path/to/fir/v1/frontend; see the [example](compiler/PARSER_COVERAGE.md).
The CMake toolchain/system/processor/compiler options above also apply to this
C99 project. Coco/R stays a HOST tool. Libraries and CLIs must be built for the
target platform, not copied from the host build. Carry the common MIT LICENSE
and generated-frame notices when detaching a component.

[0.6.0: Unicode, regexp, CONFIG, Vector, generator boundary](compiler/LEXICAL_PROFILE.md).
