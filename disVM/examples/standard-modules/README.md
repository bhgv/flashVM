# Real Dis standard-module tutorial / Проверка модулей на Dis

[Source, every line explained](main.b) · [Module status and limitations](../../libraries/README.md) ·
[Native runtime](../../runtime/README.md) · [Root build menu](../../../README-FIRSTSTEPS.md)

Build with `python3 build_menu.py`, select `all` or `disvm-runtime`, then launch
**7**. The installer places the program, debug symbols, matching runtime/modules,
source and guides together in out/disvm-runtime. There is no external emu.
Alternatively run `sh out/disvm-runtime/run-modules.sh`; --no-jit is an optional
developer comparison, not a required menu flag. Expected final lines contain
`PASS standard modules and native-cell tables` and `PASS Dis application completed`.

The program executes actual retained libraries: Bufio reads Unicode including a
non-BMP character with pushback/EOF; String parses text; Arg consumes options and
returns its positional list exactly once; Srv repeatedly resolves numeric loopback
and checked service ports; Math computes scalars/vectors, sorts a permutation and
sign-extends wire int32; Styx packs/unpacks a real 9P2000 Read with a 64-bit offset;
Styxservers handles EOF; Readdir/Debug initialize their admitted pure services;
Loader returns actual instructions, descriptors and exports of this module.
Sparse int, big and dense computed-jump branches execute, and Loader verifies
that ICASE, ICASEL and IGOTO are really present in the emitted program.

Malformed Styx tests cover every incomplete prefix and advertised sizes below
the fixed header in both request/response decoders. No source/user file is
created or overwritten, no external DNS or network connection is required, and
no graphics module is loaded. Numeric loopback uses the real native resolver.
Arrays/lists/modules follow normal Dis ownership. Bufio closes its in-memory
reader; the process exits through the runtime's ordinary scheduler/GC lifecycle.
The runtime owns native resolver synchronization until OS process teardown.

This is executable acceptance for those calls on Linux AMD64 NC64. It is not a
full Styx server, namespace, filesystem browser, source debugger or Math conformance
suite. /prog and several Sys services are still unavailable. Scalar Math uses
native libm; unchanged Inferno sources and private adaptations retain provenance.
The matching symbols/bytecode are specific to this experimental ABI.

По-русски: выберите пункт **7** после сборки. Пример подробно комментирует каждую
строку и исполняет настоящие модули, включая Srv/Math/Loader и Styx. Внешний emu,
сеть и графическая среда не нужны; пользовательские файлы не меняются. Проверки
не объявляют готовыми /prog, полноценную VFS, сетевой Styx-сервер или графическую IDE.
