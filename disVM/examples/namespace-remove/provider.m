# Copyright (c) 2026 bhgv
# EN: Typed provider loaded through independent namespace groups.
# RU: Типизированный provider для загрузки через независимые namespace.
NamespaceProvider: module {
    tag: fn(): int; # Immutable code identity, independent of instance MP.
    run: fn(value: int): int; # Verify private MP across a native yield.
};
