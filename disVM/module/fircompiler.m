# Optional native library service; no shell, subprocess or UI authority.
Fircompiler: module
{
    PATH: con "$Fircompiler"; # Builtin present only in explicitly linked compiler hosts.
    AS1, # Numeric ActionScript 1 compatibility profile.
    AS2, # Numeric ActionScript 2 compatibility profile.
    AS3, # Numeric ActionScript 3 profile targeting AVM1 compatibility.
    Limbo: con iota + 1; # Bounded executable Limbo source profiles.
    Dis, # Complete NC64 Dis module for this runtime contract.
    Fir, # Canonical textual FIR after optional optimization.
    Swf: con iota + 1; # Complete SWF8 linked from the private SWO.
    Ok, # Complete independent artifact and debug bytes.
    Invalid, # Invalid language, target or optimization option.
    Source, # Source frontend failure; inspect the copied diagnostic kind/span.
    Verify, # FIR reader/verifier/optimizer failure.
    Backend, # Dis/object/linker rejection.
    Resource, # Allocation or temporary/output I/O failure.
    Limit: con iota; # Source or output byte budget exceeded.
    MaxSource: con 65536; # Exact UTF-8 byte budget before any source parsing.
    MaxResult: con 2097152; # Independent artifact and debug byte budgets.

    # Serialized, blocking native call. Input stays rooted/immutable until return.
    # Returns stage, fresh artifact/debug arrays, message, source diagnostic kind,
    # one-based line, zero-based UTF-8 byte offset, byte length. Missing location
    # uses line=0, offset=-1, length=0. Failure returns nil arrays; no partial build.
    # optimize is exactly 0/1; language/target use constants above. Debug is FIRDBG1
    # before optimization, not a final target PC map. There is no cancellation.
    compile: fn(language, target, optimize: int, source: array of byte):
        (int, array of byte, array of byte, string, int, int, int, int);
};
