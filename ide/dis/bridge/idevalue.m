# Explicit copied values and owner references, independent of either VM's native layout.
Idevalue: module
{
    PATH: con "idevalue.dis"; # Pure bounded conversion/copy module.
    Undefined, # Missing dynamic value, distinct from Null.
    Null,      # Explicit null scalar.
    Bool,      # Canonical zero or one.
    Int,       # Checked signed integer of the selected Limbo int profile (32 or NC64 64).
    Big,       # Exact signed 64-bit value; never encoded as AS Number.
    Real,      # Binary64 scalar; conversions do not execute application code.
    Char,      # One Unicode scalar, excluding surrogate code points.
    Text,      # Copied Unicode string.
    Table,     # Explicit shallow named snapshot; not an executable object.
    Object,    # Owner/object identity descriptor, never a heap pointer.
    Method,    # Bound method descriptor retaining its receiver identity.
    Byte: con iota; # Checked unsigned 8-bit scalar, distinct from a Unicode Char.
    MaxText: con 4096; # Per-text/per-name Unicode character limit.
    MaxFields: con 32; # Maximum explicit snapshot entries.
    MaxTableText: con 16384; # Aggregate names and string values in a snapshot.


    # Reflected conversion target, derived from the compiled Limbo profile.
    Type: adt
    {
        kind: int; # Canonical Value discriminant.
        name: string; # Client request type, e.g. real rather than platform-dependent float.
        base: string; # Actual Limbo storage type, or data snapshot for Table.
        helper: string; # Optional AS library helper name, never a native builtin claim.
        bits: int; # Numeric payload width, zero for variable-length strings/tables.
        copied: int; # One for value/snapshot, zero for Object/Method owner descriptors.
    };


    # Transport-neutral descriptor. Only the fields belonging to kind are meaningful.
    Value: adt
    {
        kind: int; # One discriminant above; invalid tags are rejected before publication.
        integer: big; # Bool/Int/Big/Char payload; no lossy real conversion for Big.
        number: real; # Real payload, including IEEE NaN/Inf/signed zero.
        text: string; # Text payload, copied using immutable Limbo string semantics.
        owner: string; # Object/Method logical receiver identity; not network authority.
        epoch: int; # Object/Method positive receiver incarnation.
        member: string; # Method's exact name; empty for an Object.
        names: array of string; # Table's copied ordered unique names.
        values: cyclic array of ref Value; # Copied scalar/reference entries; nested tables are refused.
    };


    # Validate and copy one value; tables get independent arrays/records, references keep identity.
    # value is borrowed until return; returns nil for invalid/quota/cyclic/nested table input.
    copy: fn(value: ref Value): ref Value;


    # Return this compiled module target's int width, 32 or 64; no host pointer sizes are exposed.
    intbits: fn(): int;


    # Return a fresh reflected catalog for this compiled target; no application object is inspected.
    # Scalars follow Limbo types; bool/char/table explicitly identify their extension representation.
    types: fn(): array of ref Type;


    # Produce a copied scalar conversion. target uses the discriminants above.
    # Strict Int/Big/Char/Byte conversions reject fractions, overflow and malformed decimal strings.
    # Real is binary64: explicit Big to Real may round; decimal text must be finite and complete.
    # Object/Method conversions require their owner; this helper never evaluates user code.
    # Returns nil on unsupported/invalid conversion; identity conversion still returns a copy.
    convert: fn(value: ref Value, target: int): ref Value;
};
