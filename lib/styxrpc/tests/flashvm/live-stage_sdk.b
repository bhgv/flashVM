# Copyright (c) 2026 bhgv
# EN: Publish an aliased array and a nil string to the standalone C SDK over real Styx.
# RU: Опубликовать массив с алиасами и nil-строку самостоятельному SDK C по Styx.
implement Client;
include "sys.m";
include "vfsexport.m";
include "bigtable.m";

Client: module { init: fn(); };


# EN: Retain the owner capabilities until the C client confirms the changes.
# RU: Удерживать возможности владельца до подтверждения изменений клиентом C.
init()
{
    sys := load Sys Sys->PATH;
    exports := load Vfsexport Vfsexport->PATH;
    cell := exports->intvar(21);
    numbers := exports->intarray(cell :: cell :: nil);
    name := exports->stringvar(nil);
    wide := exports->bigvar(big "-9223372036854775808");
    tables := load Bigtable Bigtable->PATH;
    hybrid := tables->create();
    if (tables->push(hybrid, big 11) != tables->Ok ||
        tables->addindex(hybrid, big 9, big 19) != tables->Ok) {
        raise "could not seed the numeric table projection";
    }
    table := exports->table(hybrid);
    root := exports->record(("numbers", numbers) :: ("name", name) :: ("wide", wide) :: ("table", table) :: nil);
    if (root == nil || exports->mount(root, "/live/.var", Sys->MREPL, 0) != 0) {
        raise "could not mount the SDK live variables";
    }
    began := sys->millisec();
    permit := array[1] of byte;
    for (;;) {
        marker := sys->open("/sdk-done", Sys->OREAD);
        if (marker != nil && sys->read(marker, permit, 1) == 1 && permit[0] == byte '1') {
            break;
        }
        if (sys->millisec() - began >= 5000) {
            raise "no confirmed SDK readback";
        }
        sys->sleep(5);
    }
    (ok, value) := exports->intget(cell);
    (tableok, numeric) := tables->getindex(hybrid, big 9);
    if (!ok || value != 63 || tableok != tables->Ok || numeric != big "9223372036854775807") {
        raise "C write missed the native aliased Int32 or numeric table key";
    }
    sys->print("PASS DisVM observed C SDK writes\n");
}
