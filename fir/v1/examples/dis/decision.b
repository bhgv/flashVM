# Our Coco/R numeric Limbo profile, using a terminal decision tree without SSA merges.
implement Decision;


Decision: module
{
    value: fn(): int; # The tutorial adapter captures the selected return value in FIR @main.
};


# Select one signed i32 result after checking an overflowing intermediate.
# No arguments or heap/module owners. Each arm returns, so no mutable binding crosses a join.
value(): int
{
    wrapped := 2147483647 + 1; # Normalize now to -2147483648, before either comparison.
    if (wrapped >= 0) {
        return -1; # Static capture slot zero exists, but this path is not visited.
    } else {
        if (wrapped == -2147483648) {
            return 42; # Static capture slot one executes; its flag distinguishes it from zero-filled slots.
        } else {
            return 0; # A legitimate result of zero would have flag one if this path were visited.
        }
    }
}
