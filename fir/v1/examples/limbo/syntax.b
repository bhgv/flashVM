# Native Limbo radix/inference example.
implement V;
V: module { value: fn(): int; };
value(): int {
    x := 2r1010;
    y: int = 16r20;
    x += y;
    x *= -1;
    return -(x);
}
