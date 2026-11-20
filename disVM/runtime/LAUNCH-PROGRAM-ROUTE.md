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

If the selected process exports `.vfs`, preparation captures that directory.
Only a confirmed absence permits inheritance of the base process's entire
`.vfs`. A non-directory or failed lookup denies preparation. Before `init`,
DisVM installs the captured directory at `/.vfs` in the new process. Existing
explicit mounts at or under that path cause adoption to fail. Local `spawn`
inherits the namespace and ordinary file permissions. Client environment
entries are ordinary files such as `/.vfs/env/CLIENT`. The execution provider
keeps its accepted root and launch context alongside this subtree.

The fixture tests an explicit client namespace and an inherited base namespace.
Each case reads a distinct environment value from `/.vfs` in both the launched
process and its child.

The planned proxy layer for `.var` exposes shared variable proxies between
Limbo/DisVM and FlashVM. Composite values appear as directories. `.type`
and `.len` provide type and length. For indexed access the client opens
`.idx`, writes the index, operates on `.val`, and closes `.idx`.
The dispatcher must associate selection with the open operation so concurrent
accesses retain separate indices. `.ins`, `.add`, `.push` and `.pop`
are planned collection operations.

On launch, DisVM projects `/.prog/.self/.pid` and `/.prog/.self/.vfs`
into the new process's private namespace. The first path reads the selected
client's read-only `.pid` file. The publisher uses a decimal PID followed
by a newline. The second path exposes the same effective tree as `/.vfs`,
including the inherited case. Environment entries remain ordinary files
under `.vfs/env`. The binding belongs to the launch namespace and survives
local `spawn`. Numbered descriptors are available through `Sys.fildes`.
If the selected process publishes a `.var` directory, it is available via
`/.prog/.self/.var` with the original Styx permissions. Reads and writes
go directly to the published files. An absent directory does not fall back
to the base process. The fixture checks updates to `counter` by the launched
program and its child. Automatic VM variable publication and composite
value operations remain separate work.

The fixture uses the [callback-backed integer proxy](../native/terminal/VARIABLE-FILE.md).
