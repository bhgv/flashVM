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
    # Copy all keys in deterministic order; allocation failure raises, never returns a partial list.
    keysof: fn(tab: ref Handle): ref Keys;
    # Return snapshot length and an indexed key; keyat raises on invalid bounds/foreign owners.
    keylen: fn(keys: ref Keys): big;
    keyat: fn(keys: ref Keys, index: big): (int, big, string);


    # Checked key capabilities retain the immutable snapshot, never a native address.
    keycheck: fn(keys: ref Keys, index: big);
    getkey: fn(tab: ref Handle, keys: ref Keys, index: big): (int, big);
    setkey: fn(tab: ref Handle, keys: ref Keys, index: big, value: big): int;


    # Compiler helpers preserve eager errors while using existing Dis module-call instructions.
    requireok: fn(status: int); # Raise on a rejected get/set result; successful zero remains silent.
    indexcheck: fn(key: big); # Reject a negative numeric key before an unused location can escape.
    rangecheck: fn(first: big, end: big); # Reject negative/reversed bounds independently of slice use.
};
