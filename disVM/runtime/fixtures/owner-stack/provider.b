# Copyright (c) 2026 bhgv
# EN/RU: Real local recursion inside a separately loaded provider module.
# Настоящая local recursion внутри отдельно загруженного provider module.
implement StackProvider;
include "provider.m";


# EN/RU: Keep depth+1 recursive frames alive across an ordinary channel receive.
# Сохранить depth+1 recursive frames на время обычного channel receive.
recurse(depth: int, ready, gate: chan of int): int
{
    if (depth > 0) {
        return 1 + recurse(depth - 1, ready, gate);
    }
    ready <-= 1;
    return <-gate;
}


# EN/RU: Export a non-tail wrapper, preserving the imported call and local return PCs.
# Экспортировать non-tail wrapper, сохраняя imported call и local return PCs.
wait(depth: int, ready, gate: chan of int): int
{
    return 1 + recurse(depth, ready, gate);
}
