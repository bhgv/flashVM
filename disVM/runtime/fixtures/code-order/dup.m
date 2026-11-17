# Copyright (c) 2026 bhgv
# EN: Same-name fixture module with a separately loaded dependency.
# RU: Same-name fixture module с отдельно загружаемой dependency.
Dup: module
{
    # EN/RU: Return this module image's marker; no I/O or mutation.
    # Вернуть marker данного module image; без I/O/mutation.
    tag: fn(): string;


    # EN/RU: Load a dependency through ordinary load, returning its marker.
    # Загрузить dependency через обычный load и вернуть её marker.
    dependent: fn(): string;
};
