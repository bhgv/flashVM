# Numeric Limbo tutorial: lexical ownership, not a general Dis loader module.
implement Scopes;                         # Match the public tutorial interface below.
Scopes: module { value: fn(): int; };     # One numeric entry lowered to FIR @main.
value(): int                             # The source adapter traces this return value.
{
    x := 5;                              # Outer x remains visible after nested blocks.
    total := 0;                          # Writes to this outer binding will persist.
    {                                    # Enter a lexical scope with its own names.
        x := x + 2;                      # Initializer reads outer x, then shadows it with 7.
        total = x * 6;                   # Update outer total to 42, using local x.
        {                                # Start another independent lexical owner.
            x: int = 1;                  # Typed declaration shadows the x holding 7.
            total += x;                  # Preserve outer total=43 after leaving both scopes.
        }                                # Retire x=1; x=7 becomes visible again.
    }                                    # Retire x=7; outer x=5 is visible again.
    return x + total;                    # 5+43=48, using i32 wrap arithmetic.
}
