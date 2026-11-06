Provider: module
{
    wait: fn(): int;
    touch: fn(): int;
    failheld: fn(value: array of int);
    pendingfail: fn(): int;
    pendinguse: fn(value: array of int, n: int): int;
    heldvalue: fn(value: array of int): int;
    spawned: fn(done: chan of int);
};
