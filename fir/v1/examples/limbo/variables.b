implement V;
V: module { value: fn(): int; };
value(): int {
    x: int = 6;
    y: int = x * 7;
    x = y - x;
    return x;
}
