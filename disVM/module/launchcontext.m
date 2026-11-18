# Copyright (c) 2026 bhgv
# EN: Read-only copied startup data for the current accepted launch, independent of display/VFS paths.
# RU: Read-only копии стартовых данных своего принятого запуска, независимо от дисплея и VFS-путей.
Launchcontext: module
{
    PATH: con "$Launchcontext"; # Matched hosted builtin; standard Inferno Sys stays unchanged.

    # EN/RU: Copy current launch's logical module label; nil without an accepted handoff.
    # Скопировать logical module label своего launch; nil без принятого handoff.
    # This is description, not permission or an implicit host pathname lookup.
    # Это описание, не полномочие и не неявный поиск host pathname.
    name: fn(): string;

    # EN/RU: Independent mutable byte copy, at most65536 bytes; nil for absent/empty arguments.
    # Независимая изменяемая копия bytes, не более65536; nil для отсутствующих/пустых arguments.
    # Encoding belongs to the caller/bootstrap; no pointers or automatic decoding.
    # Encoding задаёт caller/bootstrap; без pointers и автоматического decoding.
    arguments: fn(): array of byte;
};
