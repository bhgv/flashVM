# Client process context on a Styx route

[Русский](LAUNCH-PROGRAM-ROUTE.ru.md) · [Example launcher](test_unibus_files.c) · [Numbered FDs](LAUNCH-PROGRAM-FDS.md)

The launcher sets `DVM_LaunchSpec.program_root` to the granted client-side
directory of a particular loaded module instance. A request selects its
process within that directory. The same PID can refer to different contexts
on different module loads.

## Launch record

`styxterminallaunchprogramaddress` retains the request file and copies the
`DVM_LaunchProgram` selector. It prefixes guest arguments in the existing
S-record with a 13-byte header.

| Bytes | Field |
| --- | --- |
| 0 | Version 1 |
| 1–4 | Nonzero client PID |
| 5–8 | Base PID, zero for no inheritance |
| 9–12 | Mask of hidden FD numbers |
| 13 onward | Original startup bytes |

The integer fields following the version are big-endian. The record
accepts up to 65,536 payload bytes including the prefix. With a configured
`program_root`, the receiver rejects an unsupported version, a truncated
prefix, a zero PID, or a PID equal to its base. Preparation captures
`.prog/<pid>` and its optional base before publishing READY. A missing
directory denies the request. `Launchcontext.arguments()` returns the
original bytes, including embedded NULs. Routes without `program_root`
continue to accept their original argument format.

## Process namespace

The receiver imports `<load>/.prog/<pid>/.fd`. The external Styx fixture
exports `loaded.dis/.prog/41` and `loaded.dis/.prog/42`. Run
`ctest -R '^terminal-ordered-files-guest$' -V` and look for
`PASS actual portable Dis guest record files scoped` in both VM modes.

The remaining design places `.vfs` and `.var` alongside `.fd`. The
former exposes the granted client namespace, including ordinary environment
files. The latter exposes shared variable proxies between Limbo/DisVM and
FlashVM. Composite values appear as directories. `.type` and `.len` provide
type and length. For indexed access the client opens `.idx`, writes the
index, operates on `.val`, and closes `.idx`. The dispatcher must associate
selection with the open operation so concurrent accesses retain separate
indices. `.ins`, `.add`, `.push` and `.pop` are planned collection
operations. These nodes remain to be connected.
