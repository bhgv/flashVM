Provider: module
{
    wait: fn(): int;
    touch: fn(): int;
    heldvalue: fn(value: array of int): int;
    spawned: fn(done: chan of int);
};
