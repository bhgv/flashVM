# Owner-local native implementation of the first hybrid table value specialization.
Bigtable: module
{
    PATH: con "$Bigtable"; # Hosted builtin; not an Inferno ABI extension.
    Ok, Missing, Invalid, Nomem: con iota; # Explicit read/write status.
    Index, Text: con iota; # Key namespaces; more kinds require a future ABI revision.


    # Opaque VM-managed owner. The reserved field carries no pointer or authority.
    Handle: adt
    {
        reserved: int; # Ignored by native calls; constructing this ADT does not create a table.
    };


    # Independent immutable key snapshot. Retaining it never retains the source table.
    Keys: adt
    {
        reserved: int; # Ignored field; only keysof creates an accepted native owner.
    };


    # Allocate a table with VM finalization; raises on allocation failure.
    create: fn(): ref Handle;
    # Read a typed big; consume status before using value. Missing is distinct from zero.
    getindex: fn(tab: ref Handle, key: big): (int, big);
    gettext: fn(tab: ref Handle, key: string): (int, big);
    # Copy a value and, for text, the key; failure leaves logical contents unchanged.
    setindex: fn(tab: ref Handle, key: big, value: big): int;
    settext: fn(tab: ref Handle, key: string, value: big): int;
    # Remove one entry; return Missing when absent. No implicit sequence compaction.
    delindex: fn(tab: ref Handle, key: big): int;
    deltext: fn(tab: ref Handle, key: string): int;
    # Return contiguous sequence length, independently of total pair count.
    length: fn(tab: ref Handle): big;
    count: fn(tab: ref Handle): big;
    # Copy a checked half-open sequence slice, reindexed from zero; raises on invalid bounds/OOM.
    slice: fn(tab: ref Handle, first: big, end: big): ref Handle;
    # Normalize relative bounds before RHS; returns absolute exclusive bounds.
    slicerange: fn(tab: ref Handle, first: big, end: big): (big, big);
    # Replace a captured range transactionally with replacement's dense prefix.
    splice: fn(tab: ref Handle, first: big, end: big, replacement: ref Handle): int;
    # String bounds count Unicode runes; replacement returns independent storage.
    stringrange: fn(text: string, first: big, end: big): (big, big);
    stringsplice: fn(text: string, first: big, end: big, replacement: string): string;
    stringindex: fn(text: string, index: int): (big, big); # One checked rune destination.
    runetext: fn(value: int): string; # Reject non-scalars, including surrogate code points.
    # Copy all keys in deterministic order; allocation failure raises, never returns a partial list.
    keysof: fn(tab: ref Handle): ref Keys;
    # Return snapshot length and an indexed key; keyat raises on invalid bounds/foreign owners.
    keylen: fn(keys: ref Keys): big;
    keyat: fn(keys: ref Keys, index: big): (int, big, string);


    # Reflection returns Limbo type names without converting values or invoking user code.
    BigKind, StringKind, TableKind, KeysKind: con iota; # Stable helper tags, not FIR enum IDs.
    # Explicit copied conversions; decimal text must be complete and fit signed big.
    bigfromtext: fn(text: string): big;
    textfrombig: fn(value: big): string;
    keybig: fn(keys: ref Keys, index: big): big;
    keytext: fn(keys: ref Keys, index: big): string;
    typename: fn(kind: int): string; # 0 big, 1 string, 2 table, 3 keys, 4 int; no pointer/type coercion.
    keytype: fn(keys: ref Keys, index: big): string;


    # Checked key capabilities retain the immutable snapshot, never a native address.
    keycheck: fn(keys: ref Keys, index: big);
    getkey: fn(tab: ref Handle, keys: ref Keys, index: big): (int, big);
    setkey: fn(tab: ref Handle, keys: ref Keys, index: big, value: big): int;


    # Compiler helpers preserve eager errors while using existing Dis module-call instructions.
    requireok: fn(status: int); # Raise on a rejected get/set result; successful zero remains silent.
    resolveindex: fn(tab: ref Handle, key: big): big; # Capture a relative numeric key before RHS.
    indexcheck: fn(key: big); # Reject a negative numeric key before an unused location can escape.
    rangecheck: fn(first: big, end: big); # Check absolute ordering; relative bounds resolve at the receiver.

    # Normalize presence/deletion statuses: Ok -> 1, Missing -> 0; other errors raise.
    present: fn(status: int): int;
    # Delete through an immutable snapshot key; return status without compacting indices.
    delkey: fn(tab: ref Handle, keys: ref Keys, index: big): int;
};
