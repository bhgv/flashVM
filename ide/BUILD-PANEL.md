# Build panel on Dis — v0.1671

The graphical editor now builds its current document with **F7** and selects its
last source diagnostic with **F8**. Application policy executes in Limbo/Dis;
FlashVM renders the existing copied channel view. This uses the real optional
`$Fircompiler` library, without an external compiler process.

Save the document with an explicit lower-case suffix first:

| Suffix | Language | Output retained in memory |
| --- | --- | --- |
| `.b` | bounded Coco/R Limbo | Dis |
| `.as`, `.as2` | bounded Coco/R AS2 | SWF 8 |
| `.as1` | bounded Coco/R AS1 | SWF 8 |
| `.as3` | bounded Coco/R AS3 | SWF 8 |

F7 requests optimization and shows plain compiler status in the existing read-only
inspector panel. `Build status=0 stage=0` means an admitted successful build.
A source failure reports status 5 and native stage 2 with its diagnostic. Escape
returns to source. F8 may be pressed while output is visible; it selects the
original diagnostic using UTF-8 byte-to-character conversion in the document
owner. Source edits invalidate old diagnostic selection and artifact access.
Unknown suffixes ask for Save As; an OFF build reports the unavailable capability.
These language names describe the implemented bounded frontends, not full language
coverage. AS3 input uses the project's FIR/SWF 8 route, not an AVM2 container.

## Portable library contract

`Idebuildpanel` is a reusable ordinary Dis channel service. `start` borrows the
actual editor document endpoint and loaded `Idecore`, `Ideworkspace` and
`Idecompiler` implementations. It creates and owns a private workspace and worker;
missing native compilation returns nil after shutting down its private owner.
The graphical editor keeps working when that capability is absent.

Each non-nil reply endpoint must be consumed exactly once. Requests carry a
nonnegative ID and an exact editor revision. Build additionally carries explicit
language/target/optimization values, so library clients can select Dis, FIR or
SWF independently of the graphical defaults. Invalid requests preserve results.
There is no request cancellation or idempotency guarantee.

Build captures the actual editor snapshot, opens a private workspace source,
compiles through the existing worker, and rechecks the actual editor revision
before retaining output. Every temporary document closes after the job. A
concurrent edit rejects even a successful native result with Conflict. The last
successful artifact survives a later failed build of the same revision; its
returned language/target remain those of that successful build. A newer document
revision cannot fetch those old bytes.

Artifact returns independent binary and original FIRDBG1 arrays only on request;
ordinary replies contain metadata. Diagnostic resolves only the latest current
source span through `Idecore.Utf8Span`, with no source/history/saved-state changes.
The editor applies the resulting character selection through `Ideinput.select`.
Shutdown acknowledges private worker/workspace retirement before becoming
terminal. It never shuts down the borrowed editor document. All graphical exit
paths retire the panel before the editor and native channel registration.

## Verification and limits

`buildpanel_test.b` tests all three targets, independent array copies, invalid
options, failure preserving prior success metadata, original Unicode spans,
revision invalidation and a deterministic competing edit after native compilation.
The latter uses ordinary document channels, without timers or production hooks.
`test_graphical_build.py` drives the actual editor bytecode and FlashVM renderer
in both dispatch modes: five suffix choices, F7, F8, stale diagnostics, unchanged
source/saved state, unknown suffix and absence of automatically published files.
The installed SDK runs the panel acceptance again after relocation.

Native compilation blocks the sole VM owner; this is not a responsive background
build or cancellation implementation. The graphical panel shows a bounded
256-character diagnostic, while the service retains its full plain text. Only the
latest attempt's selectable diagnostic and last successful artifact are retained.
No files are written, loaded or run automatically. Artifact export, Run/Debug
controls, full language/module support and final target/native PC maps remain
future work. Replay rendering does not establish physical desktop/touch acceptance.

See the adjacent `COMPILER-SERVICE.md`, `WORKSPACE.md` and `SYNC.md` contracts in
the source documentation or installed SDK for compiler and synchronization details.

[Service](COMPILER-SERVICE.md) · [Workspace](WORKSPACE.md) · [Sync](../disVM/SYNC.md) · [Interface](dis/bridge/idebuildpanel.m) · [Source](dis/bridge/idebuildpanel.b) · [Tutorial](dis/examples/build-panel/README.md)
