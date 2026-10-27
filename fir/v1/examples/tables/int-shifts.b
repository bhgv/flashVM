implement IntShifts;

# A big return type selects the Coco/R big/table profile with typed int operators.
# The same source is accepted by CLI and in-process source/FIR C99 services.
IntShifts: module
{
    value: fn(): big;
};


# Extract a low-byte value and a sign flag from one signed32 packed word.
# No arguments. Tables are local VM owners; arithmetic values are copied scalars.
# The successful arm returns 42; the failure arm returns zero for SDK acceptance.
value(): big
{
    # The upper bit marks a flag; the low byte carries the value 42.
    # Store an exact big, then explicitly narrow to int when reading it back.
    packet := table of big;
    packet.word = -2147483606;     # Bit pattern 0x8000002a.
    packed := int packet.word;

    # Right shift is arithmetic: the signed high bit propagates into all positions.
    sign := packed >> int 31;     # -1, rather than the unsigned value 1.
    payload := packed & int 255;  # Keep the low byte: 42.

    # Compound updates preserve int width after each operation, including NC64.
    restored := payload;
    restored <<= int 1;           # 84.
    restored >>= int 1;           # 42.

    # All count operands are explicit int and must be in 0..31 at runtime.
    # Lazy conditions keep the invalid-shift example in the unselected branch.
    valid := (sign == int -1) && (restored == int 42);
    if (valid) {
        return big restored;     # Widen the exact signed int, without floating conversion.
    } else {
        # This expression would raise invalid argument if the branch were selected.
        # The skipped branch must not execute its checked shift or publish a trace.
        return big (int 0 << int 32);
    }
}
