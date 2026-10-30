# Graphical debugger scaffold

[Русский](DEBUGGER.ru.md) · [MDI/SDI](MDI-SDI.md)

In menu8 IDE, expand **Debug** and choose **Open panels**. Source, stack and data
panes are ordinary readonly MovieClip windows on the same desktop as source and
vector editors. Reopening reuses the owners; title-close retires only that panel.
The next opening recreates missing panels. The shared8-pane limit still applies;
if full, existing documents are retained instead of being replaced.

[Idedebug](dis/bridge/idedebug.m) is an independently scheduled typed Limbo session
owner. The graphical shell obtains copied panel content through its channel and
acknowledges shutdown. Exact session incarnation protects commands and terminal
shutdown from stale clients. Replies explicitly declare attached=0 and capabilities=0.

Attach, pause, continue, step and next currently return Unsupported without
changing state. There is **no target attachment, stepping, breakpoint execution,
stack inspection or variable evaluation yet**. Opening a pane does not execute
getters or fabricate target frames. This is the requested initial scaffold,
not Inferno deb feature parity or a claim of interpreter/JIT debug parity.

## Inferno reference and next integration

Reviewed Inferno experiment `appl/wm/deb.b`, `debdata.b`, `debsrc.b` and
`module/debug.m`: source/PC mapping, stack/frame navigation, typed value views,
run/step/statement/over/out, breakpoints and target state must be separate from
the graphical frontend. Their Draw/Tk implementation is not imported.
The selected debug/SBL library source already ships with this runtime; its
`/prog` target control/read namespace is not implemented by this host.

| Layer | Current state | Next required evidence |
| --- | --- | --- |
| MovieClip panels | Actual shared-desktop source/stack/data windows | Navigation and selection tools |
| Limbo session | Incarnation, copied snapshots, explicit Unsupported | Authorized target capability |
| Runtime | Interpreter/leaf JIT retained, matching debug symbols shipped | Owner safe-point pause and exact PC/frame capture |
| Source mapping | Existing generated symbols/library foundation | Actual target PC to source and breakpoint mapping |
| Values | No reads/evaluation admitted | Rooted copied values, stale frame guards, GC/unwind tests |
| Stepping | Disabled | Same stop semantics under interpreter/JIT |

Keep native stack/register/heap pointers out of messages. A stopped frame must
carry a target incarnation and stop epoch; resume invalidates it. Safepoints must
cooperate with the existing sole VM owner and collector, never suspend arbitrary
raster workers. New common Dis64 contracts remain subject to journal agreement.
