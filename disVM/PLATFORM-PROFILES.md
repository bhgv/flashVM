# Selected native64 toolchain profiles

[Русский](PLATFORM-PROFILES.ru.md) · [Synchronization](SYNC.md) ·
[Owning Inferno guide](https://github.com/bhgv/Inferno-OS-bhgv/blob/experiment/doc/build-native64.md)

Inferno commit daa205685c1e55acf9614dd05b603ae540d39a16 adds five mk profiles.
FlashVM retains their exact sources, target guards and small host-test closure.
These are core/library toolchain descriptions. Complete Inferno platform emulator
ports and native Android APK acceptance are separate work.

| Profile | Target selection |
| --- | --- |
| Linux-amd64 | Native/cross GCC, x86-64 LP64, `-m64` |
| Linux-arm64 | Native/cross GCC, AArch64 LP64 |
| Linux-riscv64 | Native/cross GCC, RV64GC, LP64D |
| Android-amd64 | NDK LLVM x86-64 driver, default API 21 |
| Android-arm64 | NDK LLVM AArch64 driver, default API 21 |

`native64-target.h` rejects non-LP64 or mismatched target compilers. The small
architecture lib9 headers forward to common declarations; they do not provide
machine-specific signal/FPU/cache/assembler integration. A 64-bit target does not
select a new Dis file model automatically. Optional `DISMODELFLAGS` must be
consistent across every linked runtime object and generated header.

## Reproduce host checks

Build the owned mk from its selected sources, then supply it explicitly:

```sh
sh tools/mk/bootstrap.sh build/mk
NATIVE_MK="$PWD/build/mk/mk" python3 disVM/upstream/utils/tests/native64-mkfiles.py
python3 disVM/upstream/emu/Android/tests/storage.py
cmake -S disVM -B build/core -DDVM_MK_EXECUTABLE="$PWD/build/mk/mk"
cmake --build build/core
ctest --test-dir build/core -R 'native64_mkfiles|android_storage' --output-on-failure
```

The independent core build has no hidden dependency on a sibling mk build.
`DVM_MK_EXECUTABLE` is optional: set it to enable the actual-mk profile gate.
The upstream driver also understands `NATIVE_MK`; its automatic full-Inferno
bootstrap fallback is not part of our selected DisVM snapshot. Use the explicit
input when running that driver or the selected aggregate suite.

The profile gate runs the real mk parser on all five profiles, then compiles,
links and executes matching-host WORD64/WORD32 probes and rejects a deliberately
wrong architecture selection. In this series the actual host is Linux x86-64.
ARM64/RV64/NDK compilation or physical execution is not inferred from expansion.

## Android path-resolver boundary

The independently testable POSIX `storage.c`/`.h` prefers native-launcher app
storage, supports explicit root selection and refuses invalid/overlong/unavailable
paths without publishing a partial output. Its source and 32 filesystem predicates
are retained and run under strict UBSan. The test restores environment variables
and uses temporary directories. It imports no Android SDK, Draw/Tk, JNI or full
platform emulator into the DisVM runtime.

Upstream Android startup/trace/mkfile integration and the separate companion APK
patch are documented in the owning guide. They are outside this selected helper
closure; host path tests do not prove actual APK startup or storage grants.
