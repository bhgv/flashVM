Provider: module
{
    wait: fn(): int;
    touch: fn(): int;
    spawned: fn(done: chan of int);
};
