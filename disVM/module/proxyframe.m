# Copyright (c) 2026 bhgv
# EN: Private versioned scalar/ref wire records; no native layout or public DIS64 IDs.
# RU: Приватные versioned scalar/ref wire records; без native layout и публичных DIS64 IDs.
Proxyframe: module
{
    PATH: con "proxyframe.dis"; # Matching producer's bytecode codec.
    Size: con 152; # Fixed bounded record, including header and result.
    Hello, Call, Reply: con iota + 1; # Private record tags, independent of DIS64.
    EpochHi, EpochLo, Id, Operation, Refs, Value: con 16 + 8 * iota; # Scalar offsets.
    Target, Left, Right: con 64 + 24 * iota; # Handle offsets.
    Status, Result: con 136 + 8 * iota; # Scalar result offsets.


    # EN/RU: Allocate one canonical zeroed record with kind and connection epoch.
    # Выделить canonical zeroed record с kind и connection epoch.
    make: fn(kind: int, high, low: big): array of byte;


    # EN/RU: Decode/encode eight big-endian bytes at a checked aligned offset.
    # Читать/писать восемь big-endian bytes по проверенному aligned offset.
    get: fn(bytes: array of byte, offset: int): big;
    put: fn(bytes: array of byte, offset: int, value: big);


    # EN/RU: Decode/encode a checked owner handle; no pointer or root is transmitted.
    # Читать/писать проверенный owner handle; pointer/root не передаётся.
    handle: fn(bytes: array of byte, offset: int): Proxyowner->Handle;
    sethandle: fn(bytes: array of byte, offset: int, handle: Proxyowner->Handle);


    # EN/RU: Check exact header, kind and connection epoch; no mutation or admission.
    # Проверить header, kind и connection epoch без mutation/admission.
    valid: fn(bytes: array of byte, kind: int, high, low: big): int;


    # EN/RU: Compare immutable record bytes; skip kind and result for reply correlation.
    # Сравнить immutable bytes; пропустить kind/result при reply correlation.
    same: fn(left, right: array of byte, response: int): int;


    # EN/RU: Clone request into a reply; preserve exact correlation and owner arguments.
    # Копировать request в reply с точной correlation и owner arguments.
    answer: fn(request: array of byte, status, value: int): array of byte;
};
