# Copyright (c) 2026 bhgv
# EN: Hosted VFS extension constants for ordinary Sys.bind; no loadable service.
# RU: Константы расширений hosted VFS для Sys.bind; без загружаемого сервиса.
Vfsflags: module
{
    MINS: con 16; # Indexed insertion through Vfsctl.bind, not Sys.bind.
    MAUTORWX: con 8; # Explicit access-based union selection; not an Inferno flag.
};
