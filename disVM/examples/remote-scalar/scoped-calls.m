# Copyright (c) 2026 bhgv
# EN: Compiled value and ref calls plus an unscoped control export.
# RU: Вызовы значений и ref по схеме и контрольный экспорт без контекста.
include "ref.m";

Scopedcalls: module {
    inspect: fn(fail: int): (int,big); # Scope checked by the implementation.
    update: fn(value: ref Cell): int; # Copy the scoped provider's PID into the object.
    updatepair: fn(values: (ref Cell, ref Cell)); # Update both objects, including aliases.
    baseline: fn(flag: int): int; # Check the owner's namespace outside scoped calls.
};
