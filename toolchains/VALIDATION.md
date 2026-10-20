# Cross-build evidence and limits

[Русский](VALIDATION.ru.md). Series based on GitHub master
b0884d12244c72680dd8941e46379de40e942e6a (VM v0.645).

| Target | FlashVM minimal static core | Real C89 embedding client | Frontends/FIR/optimizer/SWL | Combined C IDE client |
| --- | --- | --- | --- | --- |
| Linux ARM64, musl | built | linked | built/linked | linked |
| Linux RISC-V64, musl | built | linked | built/linked | linked |
| Windows x64, GNU ABI | built | linked | built/linked | linked |

All four syntax executables, fir1/fir1_source/fir1_opt and swl were produced per
target. The standalone projects and explicit-toolchain combined IDE host were
both exercised. ELF/PE headers are checked, not inferred from output filenames.
C/C++ compiler probes successfully detected eight-byte target pointers. These are
cross builds from Linux, not execution of any foreign-target artifact.

The Windows SDK SAPI mask fixture compiles in strict C++98 and links with the
real SDK. Its independent low/high-word oracle checks recognition bit 38 plus
reserved check bits 30/33. The earlier unmodified call site fails strict Clang
because SPFEI expands ull literals. The fixed call site retains the exact mask.
Unmodified upstream zlib's ULLONG_MAX warning is scoped on its Windows target;
first-party C89/C++98 checks remain enabled. No SDK source was edited or imported.

Host tools: CMake/CTest 4.4.4, external bhgv/coco-r-C revision
b91b65233c264d92c63a5c8cd855e7fc784bca53, clang-format 23.1.3, Zig 0.13.0 with
Clang 18.1.6. This sandbox has no usable /proc/self/exe: a host-only self-path
lookup shim was required for Zig, plus a wrapper omitting its unsupported
-Wl,-v diagnostic argument during CMake ABI probes. TMPDIR and caches were
explicitly writable. These host adaptations, SDK/tool binaries and generated
parsers are not project sources and are not shipped. Ordinary SDK installations
need their own unmodified host-tool setup; these are not stock-Zig certification.

Native C99 validation: 22 CTests (16 syntax frontends, 3 FIR, 2 SWL, 1 combined
IDE) passed after formatting/examples regeneration. Seven formatter checks pass,
and all six original grammar/prelude/C-action token streams matched after layout
changes. Native FIR/SWL tests exercise real FlashVM traces as well as object and
resource validation. No new sanitizer coverage is claimed for layout-only changes.

The VM profile used NO_GRAPHICS/NO_FONTS/NO_IMAGE_CODECS/NO_SQLITE with OS ports
and render jobs. This does not certify AGG/Cairo/SDL/fonts/media SDK variants,
Windows speech devices, Android NDK/APK/device runs, macOS SDK/signing/native runs,
foreign JIT equivalence or release/endurance acceptance. The supplied Android and
macOS commands are build instructions, not execution evidence. All nine roadmap
stages remain open. [Build instructions](../BUILDING.md).

## Final readable/cross series checks

After final changes, 22 native C99 CTests and eight selected VM/tooling CTests
passed: AVM1 number text, conversions, JIT numbers, speech bridges/Vosk parser,
version refresh, debugger tutorial and source links. The entire VM suite and
new ASan/UBSan checks were not run. ELF/PE checks cover 30 actual executables,
ten per target including a real C89 VM client and combined IDE host. Seven
formatter groups and five public-header compile-probe groups pass.

Six projects (AS1/AS2/AS3/Limbo/FIR/SWL) were independently copied/renamed into
paths with spaces. Each passed native build/its CTests and ARM64 cross build
using a separately copied SDK toolchain: 12 additional configuration/build checks.
Detached FIR used no implicit SWL/VM. All 1489 modified-document relative links
resolved; source-link freshness and ATG style checks passed.
