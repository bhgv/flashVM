# Source-built Inferno mk

[Русский](README.ru.md) · [Source manifest](UPSTREAM.json) · [Native build paths](../../BUILD-NATIVE.md)

Selected from bhgv/Inferno-OS-bhgv, experiment, at the exact revision in
[UPSTREAM.json](UPSTREAM.json). The manifest owns 83 upstream files:
POSIX/sh mk, the actually linked lib9/BIO/regexp closure, headers and notices.
The initial closure was pruned using a native link map: 58 translation units remain.
No OS, GUI, services, other utilities or prebuilt executable is imported.
Upstream C, original mkfiles and licenses remain byte-for-byte; adaptations live
outside upstream/. The host lib9 wrapper bypasses the ARM-only caller-address
inline, supplies native time declarations and uses host/support.c for diagnostics.
No ARM machine code is used in the Linux AMD64 build.

Bootstrap without CMake, make or a preinstalled mk:

```sh
sh tools/mk/bootstrap.sh build/mk
build/mk/mk -f mkfile compilers linker
```

Or one command: `./build-workspace.sh --backend mk --projects compilers,linker`.
The actual mk parses the root mkfile; recipes invoke tools/build-native.sh to
compile with CC/ar directly. Tools/mk's own mkfile can rebuild mk from that folder.
CC and AR name executable paths; CPPFLAGS/CFLAGS/LDFLAGS are whitespace-separated
option lists. Use a wrapper executable for complex compiler argument quoting.
Directories with spaces are tested. The bootstrap always rebuilds; the CMake
workspace path is incremental. Upstream warnings are preserved.

Test: `python3 tools/mk/tests/smoke.py build/mk/mk`. It executes recipes, wildcard
rules, stable timestamps and failure propagation. Equal-second timestamps may
rebuild in upstream mk; the test uses explicitly older input without sleeping.
Native host acceptance here is Linux AMD64/glibc. Physical Android, BSD, ARM64 and
RISC-V hosts have not been accepted by these build-tool tests.


Command-line assignments now use a bounded mkstemp file in the configured
TMPDIR, including directories with spaces, and remove it after parsing.
Unusable/overlong directories fail cleanly. Run the owning regression with
`python3 tools/mk/upstream/utils/tests/mk-tempdir.py build/mk/mk`.
