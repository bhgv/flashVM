# Dependency review checkpoint — 2026-10-11

This checkpoint is one dedicated Git commit, followed by a pause for review.
Unfinished standard-object work is retained separately in Git stash and is not
part of the reviewed working tree. The remote mirror has not been updated.
The source archive includes the repository's full .git history, including stash.

Changes: extern/ merged into external/; real Cairo 1.16.0, Pixman 0.38.4 and
SQLite 3.50.4 sources bundled; selected libraries compiled statically and offline;
Cairo wired to the existing RGBA8 polygon API; SQLite enabled from its bundled
amalgamation by default. DEFS.txt selects exclusive graphics/platform/output,
database, adapter and execution variants. Reserved backends fail explicitly.
The vendor archives' provenance, licenses, complete source patches and source
manifest are included. New integration code and build scripts have readable
formatting/comments; upstream is not wholesale reformatted.

| Verification on Linux x86_64, GCC/G++ 13.3.0 | Result |
| --- | --- |
| CMake Release, GRAPHICS_AGG + SQLITE | 101/101 tests and examples |
| CMake Release, GRAPHICS_CAIRO + SQLITE | 101/101 |
| CMake Release, NO_GRAPHICS + SQLITE | 101/101 |
| CMake Release, NO_GRAPHICS + NO_SQLITE + NO_OS_PORTS + NO_RENDER_JOBS | 94/94 |
| Standalone strict C89/C++98, default definitions | 101/101 |
| Standalone Cairo, ASan/UBSan | 100 tests; one additional UBSan-only test |
| DEFS.txt configuration and automatic regeneration | 20/20 cases |
| Offline vendored source manifest | 886/886 files |

All selected libraries compile in C89/C90 or C++98 with pedantic-errors;
undeclared C functions fail compilation. Assertions stay active in Release.
New tests check resolved build macros, bundled SQLite version/compile options,
64-bit bound values, SQL rollback/error recovery, Cairo version/recording and
soft-light Pixman compositing, straight-alpha source-over, padding and hidden
RGB preservation. Existing VM/GC/worker/channel tests are also exercised.

Evidence: [summary](validation/dependencies-summary.json),
[definitions](validation/dependencies-definitions.json),
[strict run](validation/dependencies-strict-linux.json),
[sanitizers](validation/dependencies-sanitizers-cairo-linux.json),
CMake configure/build/test logs named validation/dependencies-*-<profile>.log.
Linux standalone commands: python3 scripts/check.py; python3 scripts/check.py
--sanitize --defs /path/to/cairo-defs.txt. Configuration checks:
python3 scripts/check_definitions.py; manifest: python3 scripts/check_vendor_sources.py.
See [selection instructions](BUILD_DEFINITIONS.md).

Limits: LeakSanitizer is disabled because this environment lacks /proc task
access. container_exceptions.cpp replaces allocation operators and is tested
with UBSan only. Third-party native fixed-width types/compiler helpers remain;
strict language-mode compilation is not a guarantee for every historical compiler.
Windows/macOS/Android execution and cross toolchains were not tested. Pins are
compatibility/reproducibility choices, not a claim of newest releases or full
upstream security validation. Cairo is the image/recording/user-font profile;
GL/GUI/native fonts/PNG/document output and Pixman SIMD are disabled. Cairo's
premultiplied intermediate can cause different low-alpha rounding from AGG.
FB/GLES/OGL/DX/SDL1/SDL2/SDL3, JIT and media remain unimplemented. The complete
SWF <= 8 player and all standard ActionScript objects are not declared finished.
