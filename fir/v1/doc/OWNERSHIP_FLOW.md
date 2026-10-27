# Retained-owner verification across control flow

[Русский](OWNERSHIP_FLOW.ru.md) · [Assignment scopes](TABLE_LEASES.md) ·
[Commented FIR example](../examples/tables/ownership-flow.fir) ·
[Example commands](../examples/tables/README.md)

FIR 0.35 checks retained table ownership after CFG, dominance and type validation.
Every block has an exact incoming vector of owner recursion counts. Lock adds one
level; unlock requires a positive count and removes one. A normal `return` must
have zero retained levels. All incoming paths at a join or loop backedge must
agree. The verifier follows `copy` chains to the defining table owner, so an
alias can acquire/release the same recursive lease without inventing a new owner.
Distinct table constructions remain distinct even when their contents are equal.

A loop may retain an owner across iterations when its incoming and outgoing
counts agree. An extra lock per iteration is rejected at the backedge; a leave
without a matching entry is rejected. Branch arms may perform different balanced
work, but cannot carry different ownership to their shared successor. Diagnostic
messages identify the offending instruction line or join block. Validation does
not execute code or acquire any VM guard.

The check applies equally to textual CLI input and public C99 modules built by an
IDE or another compiler. Removed instructions contribute no ownership effects.
Optimization verifies both its input and result; Dis/SWO preflight fails before
publishing bytes. CLI staging also preserves an existing output file on failure.
Modules without live lock/unlock effects allocate no ownership-flow workspace.
For modules with effects, bounded private vectors are allocated and released
inside verification; an allocation failure reports a compiler resource error.

This is a balance check, not a runtime deadlock proof. It does not establish the
native address order of multiple objects or the correctness of channel protocols.
It does not add rollback, remote cancellation or proxy alias analysis. Only the
current executable profile's table identities and `copy` aliases are modeled;
future owner-producing operations must define their alias semantics explicitly.

Explicit native Limbo `Bigtable.acquire/release` remains a separate API. Runtime
exception/process-retirement cleanup is still required, even for verified FIR:
exceptions can bypass normal leaves. Native programs may deliberately retain a
lease until process exit; executable FIR uses the stricter balanced-normal-return
contract and must emit a leave on each normal return path.

From the repository root with matching built tools:

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1 check fir/v1/examples/tables/ownership-flow.fir
build/fir/fir1 opt fir/v1/examples/tables/ownership-flow.fir ownership.opt.fir
build/fir/fir1 dis ownership.opt.fir ownership.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 2 ownership.dis
build/runtime/disvm --no-jit --quantum 2048 --stats --inspect-words 0 2 ownership.dis
```

The example returns `DATA 0 42` and `DATA 8 1`, with no live table/key owners after
completion. Tests also cover raw/optimized images, recursive aliases, equal joins,
balanced loops, malformed loops, wrong-owner leaves and C99 caller mutations.
