implement DecimalAppend;
include "sys.m"; # Native output is provided by the same host as other Dis modules.
include "math.m"; # IEEE bit observation distinguishes negative zero from positive zero.

DecimalAppend: module # Parameterless entry, usable by the native NC64 launcher.
{
    init: fn(); # Run the tutorial; no files, windows or document owners are created.
};


# Convert a borrowed runtime string through the Dis instruction selected by the compiler.
# Returns a real value; guarded native execution may decline to the production C path.
convert(source: string): real
{
    return real source; # A parameter prevents replacing the demonstration with a constant fold.
}


# Demonstrate exact decimals, general-real fallback and retained string ownership.
# No parameters; native module handles remain rooted here; output can block on Sys.write.
init()
{
    sys := load Sys Sys->PATH; # Resolve native output through the ordinary typed module interface.
    math := load Math Math->PATH; # Resolve the actual IEEE conversion library, not a mock.
    if (sys == nil || math == nil) { # Missing host services are an explicit launch failure.
        raise "fail: decimal append tutorial modules"; # Never substitute successful sample output.
    }
    inputs := array[] of {"123456789", "-42", "-0", "1.25", "1e2"}; # Last two use general parsing.
    expected := array[] of {123456789.0, -42.0, 0.0, 1.25, 100.0}; # Independently specified numeric answers.
    for (index := 0; index < len inputs; index++) { # Each array element becomes a real runtime operand.
        value := convert(inputs[index]); # Execute Dis conversion instead of parsing in the launcher.
        negativezero := value == 0.0 && math->realbits64(value) < big 0; # Observe the IEEE sign bit.
        if (value != expected[index] || negativezero != (index == 2)) { # Compare value and signed zero separately.
            raise "fail: decimal append tutorial conversion"; # Propagate a semantic failure to the process exit.
        }
        sys->print("real %s => %g; negative zero=%d\n", inputs[index], value, negativezero); # Actual computed output.
    }
    text := "AB"; # Start with an ASCII representation owned by the Dis heap.
    retained := text; # Retain the original value so append cannot corrupt another string owner.
    text[len text] = 'C'; # INSC appends; unique-capacity reuse or shared-owner C fallback is an implementation choice.
    text[len text] = 16r20ac; # Euro requires Rune promotion when ASCII storage cannot represent it.
    text[len text] = 16r1f600; # A non-BMP scalar still occupies one Limbo string position.
    if (retained != "AB" || text != "ABC€😀" || len text != 5) { # Check ownership, Unicode contents and scalar length.
        raise "fail: decimal append tutorial ownership"; # Native counters alone are not a correctness oracle.
    }
    sys->print("retained=%s; appended=%s; scalars=%d\n", retained, text, len text); # Display both live values.
    sys->print("PASS Dis decimal/Unicode append tutorial\n"); # Emitted only after all real operations pass.
}
