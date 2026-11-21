# Copyright (c) 2026 bhgv
# EN: One compiled value call and one unscoped control export.
# RU: Один вызов по скомпилированной схеме и один контрольный экспорт без контекста.
Scopedcalls: module {
    inspect: fn(fail: int): (int,big); # Scope checked by the implementation.
    baseline: fn(flag: int): int; # Check the owner's namespace outside scoped calls.
};
