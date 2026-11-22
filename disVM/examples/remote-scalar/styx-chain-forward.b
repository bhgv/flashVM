# Copyright (c) 2026 bhgv
# EN: B invokes C's library from inside A's active library call.
# RU: B вызывает библиотеку C внутри библиотечного вызова от A.
implement Chainfd;
include "chain-fd.m";
include "sys.m";
include "vfsexport.m";


# EN: Use the same typed interface on the library mounted from C.
# RU: Использовать тот же типизированный интерфейс библиотеки с C.
forward(input, output: int): int
{
    next := load Chainfd "/remote-libs/mounted/chain-fd.dis";
    if (next == nil) {
        raise "nested library could not load C";
    }
    if (input == -3 && output == -3) {
        return check_override(next);
    }
    return next->forward(input, output);
}

# EN: Rebind one local path and export different B resources on the next hop.
# RU: Привязать другой файл по локальному пути и выдать ресурсы B следующему звену.
check_override(next: Chainfd): int
{
    sys := load Sys Sys->PATH;
    exports := load Vfsexport Vfsexport->PATH;
    original := chain_context_path(sys);
    held := sys->open(original+"/.fd/7", Sys->ORDWR);
    if (held == nil) {
        raise "B did not get the caller's numbered FD";
    }
    chain_var(sys, 25, -1);
    alternate := exports->intvar(61);
    vars := exports->intvar(51);
    files := exports->overlay(nil, alternate, "/7", Vfsexport->Read, 0);
    if (files == nil || exports->mount(files, "/.fd", 0, 0) != 0 ||
        exports->mount(exports->record(("chain-var", vars) :: nil), "/.var", 0, 0) != 0 ||
        sys->bind("/.fd/7", "/.prog/.self/.fd/7", Sys->MREPL) < 0) {
        raise sys->sprint("cannot configure B's scoped override: %r");
    }
    replacement := sys->open("/.prog/.self/.fd/7", Sys->OREAD);
    data := array[4] of byte;
    if (replacement == nil || sys->readn(replacement, data, 4) != 4 || data[3] != byte 61 ||
        sys->open("/.prog/.self/.fd/7", Sys->OWRITE) != nil ||
        sys->open(original+"/.fd/7", Sys->ORDWR) == nil ||
        sys->read(held, data, 1) != 0) {
        raise "B's bind changed a different path or an already-open FD";
    }
    if (next->forward(-3, -3) != 88) {
        raise "C did not see the new B grant";
    }
    (valid, current) := exports->intget(vars);
    if (!valid || current != 52) {
        raise "C did not update B's shared cell";
    }
    chain_var(sys, 25, -1);
    return 88;
}

include "chain-context.b";
include "chain-var.b";
