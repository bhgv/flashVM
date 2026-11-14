# Copyright (c) 2026 bhgv
# EN/RU: Typed recursive provider used to inspect cross-module return positions.
# Typed recursive provider для проверки cross-module return positions.
StackProvider: module
{
    # EN/RU: Recurse depth non-tail frames, signal ready, return gate value plus depth.
    # Создать depth non-tail frames, сообщить ready, вернуть gate value плюс depth.
    wait: fn(depth: int, ready, gate: chan of int): int;
};
