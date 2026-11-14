# Copyright (c) 2026 bhgv
# EN: Provider A keeps its instance data across a genuine native wait.
# RU: Provider A сохраняет instance data при настоящем native wait.
implement NamespaceProvider;
include "sys.m";
include "provider.m";
saved: int; # Private scalar in this Modlink's MP.


# EN/RU: Return immutable code marker; no parameters or allocation.
# Вернуть immutable code marker; без параметров и allocation.
tag(): int
{
    return 11;
}


# EN/RU: Save value in private MP, yield, verify and return value+tag.
# Сохранить value в private MP, уступить, проверить и вернуть value+tag.
run(value: int): int
{
    sys := load Sys Sys->PATH;
    saved = value;
    own := load NamespaceProvider "$self";
    if (own == nil || own->tag() != 11 || sys->sleep(0) != 0 || saved != value) {
        raise "provider A instance or self link changed";
    }
    return saved + 11;
}
