# Copyright (c) 2026 bhgv
# EN/RU: Interface shared with namespace-load tutorial providers.
# Интерфейс providers из namespace-load tutorial.
NamespaceProvider: module {
    tag: fn(): int; # Immutable code marker.
    run: fn(value: int): int; # Private MP/native-yield contract of the provider tutorial.
};
