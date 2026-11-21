# Copyright (c) 2026 bhgv
# EN: Publish three compatible ref layouts for a single owner instance.
# RU: Опубликовать три совместимые раскладки ref одного экземпляра владельца.
include "ref.m";

Compiledref: module {
    bump: fn(value: ref Cell): int; # Increment one object and return the call count.
    revise: fn(values: (ref Cell, ref Cell)); # Revise both arguments without a result.
    inspect: fn(values: (ref Cell, ref Cell)): (int,big); # Observe alias identity and the call count.
};
