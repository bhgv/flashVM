# Private native64 caller ownership

[Русский](NATIVE64-CALLER.ru.md) · [Synchronization](SYNC.md)

The selected source at `6d77333be50815c3ededb55e9b308c65b4bb623e`
incorporates Inferno journal entries INFERNO-0049 through INFERNO-0051.
FLASHVM-0049 records the review and the independent reverse contribution.

`DIS_NATIVE64_CALLER_GUARD=1` is an explicit owning-runtime option requiring
WORD32 and the fixed Math call profile. It is **not enabled by the ordinary
FlashVM embedding**. Availability markers require coherent scheduler, executor
and stack objects. Do not stub those markers to enable production calls.

A lease retains the current ready scheduler owner and its frame descriptor for
one synchronous trusted C operation. The lease has an address identity and a
saturating generation. A copied lease cannot resolve or release the original.
Cancellation, execution and stack mutations invalidate the captured generation;
an exhausted generation cannot acquire again. Clear detaches before releasing
the hold and descriptor once. Rebinding validates the replacement extent first,
then updates the saved frame and live interpreter frame together.

Interpreter entry binds the actual live `R`; ordinary return and exceptional
vmachine exits publish/detach it. Scalar Math calls validate saved/live ownership
and frame, stack and module coherence before late result resolution. Guarded
objects must be rebuilt together: `Prog` now has unconditional private epoch,
hold and live-register fields. These fields never enter a Dis/SWF/FIR file.

This freezes the **current synchronous owner**. It does not retain an arbitrary
detached process, allow a callback to block, root a general argument graph, or
authorize keeping a stack result across an asynchronous job. Existing immutable
geometry snapshots and per-pane bulk senders remain separate mechanisms.

## Independent checks

The new selected `libinterp/tests/native64-caller-sequences.py` executes the
actual private header against an independent small-state model. Eight fixture
operations, all length-five sequences, published/live modes and two WORD32
profiles give 131,072 traces per optimizer run. Four compiled removals must
fail a named semantic predicate. O1 and O3/NDEBUG strict UBSan passed on x64.
The fixture stubs scheduler availability and descriptor retirement: it proves
header transitions, not the real scheduler or GC by itself.

Run the selected checks from the FlashVM root with writable scratch:

```sh
TMPDIR="$PWD/build/core1717/tmp" \
    python3 disVM/upstream/libinterp/tests/native64-caller-sequences.py
TMPDIR="$PWD/build/core1717/tmp" \
    python3 disVM/upstream/utils/tests/native64-live-vm.py
```

The sequence check also belongs to standalone DisVM CTest on native Linux64;
cross configurations do not run a host ABI oracle as target evidence. The live
VM driver executes actual guarded executor/vmachine bodies, both ordinary and
file profiles, pilots and error exits. It does not open file native calls.

Separately, a complete owning Inferno clean-source build was reproduced on x64
using source-built mk, limbo, iyacc and decgen. Its `native64-port.py` driver
passed JIT0/1 pilot42/init42 console boots, legacy/$Sys/$Math refusals, partial
guard link refusals, header dependency invalidation, both port Math/GC lease
gates and live interpreter return/unwind gates. Eight further caller mutation
controls passed in both WORD32 profiles. That driver needs the complete owning
repository, intentionally outside this selected snapshot:

The clean build used `c958911cc2da9ad796638c4df85334d55999ba82`;
the final pin adds only the independent oracle, its README and FLASHVM-0049.
Production source blobs are identical between those two revisions.

```sh
NPROC=4 python3 utils/tests/native64-port.py \
    --build-root /writable/native-port \
    --host-tools /writable/source-built-tools --arch amd64 \
    --test-cflags '-O3 -g -DNDEBUG -fno-strict-aliasing -fsanitize=undefined -fno-sanitize-recover=all'
```

The command above runs in the owning Inferno checkout; the tools directory must
contain freshly source-built mk/limbo/iyacc/decgen. A native64 console pilot is
not full shell startup. The peer reports ARM64 gates; this series independently
ran x64 only. Physical ARM64/RV64 and the older optimized UTF/unwind ASan findings
are not closed by these UBSan checks.

## Next shared ABI question

The current scalar call's field kind and offset remain trusted C/compiler input.
A non-reference map bit cannot distinguish Real, Big or padding. FLASHVM-0049
proposes an exact compiler field witness tied to a retained Type/module/callable
generation, with field ordinal, kind, offset and width and the existing structural
signature. It is a proposal for explicit peer agreement, not a newly accepted
wire format. The 43-operation np0 file profile, `execution_supported=false`,
closed file CALL/MCALL, and all existing IDs remain unchanged.
