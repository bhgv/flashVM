# Copyright (c) 2026 bhgv
# EN: Check that ordinary load consumes the owner's real compiled profile.
# RU: Проверка того, что обычный load использует фактический профиль владельца.
implement Client;
include "sys.m";
include "blend.m";

Client: module { init: fn(); };
ScalarBlend: module { blend: fn(value: int): int; };


# EN: Try two compiled imports at the same mounted name and call the compatible one.
# RU: Проверить два скомпилированных импорта по одному пути и вызвать совместимый.
init()
{
    sys := load Sys Sys->PATH;
    path := "/peer/mounted/blend.dis";
    wrong := load Blend path;
    if (wrong != nil) {
        raise "owner profile advertised the wrong compiled signature";
    }
    actual := load ScalarBlend path;
    if (actual == nil || actual->blend(40) != 42) {
        raise "owner profile did not bind its real export";
    }
    sys->print("PASS Styx profile from loaded export\n");
}
