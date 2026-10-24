# One shared interface for either a Dis implementation or an external AS/C module.
# The host resolves this complete descriptor before binding any callable endpoint.
Arithmetic: module
{
    # Signed 32-bit inputs/result; an external AS implementation must wrap explicitly.
    add: fn(a, b: int): int;

    # Strings are values marshaled by the bridge, never pointers into another VM heap.
    greet: fn(name: string): string;
};
