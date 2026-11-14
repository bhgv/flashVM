# Copyright (c) 2026 bhgv
# EN: Terminal reply publication must already reject an immediate next call.
# RU: При terminal reply publication следующий немедленный call уже запрещён.
implement TerminalPublication;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
TerminalPublication: module { init: fn(); };
net: Proxynet; # Immutable matched network interface.


# EN/RU: Own one source connection through its natural terminal broker completion.
# Владеть source connection до natural terminal completion broker.
connect(fd: ref Sys->FD, ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image), done: chan of int)
{
    net->start(fd, fd, big 12345, big 67890, 10000, ready);
    done <-= 1;
}


# EN: Borrow kind (zero scalar, one result); return whether immediate reentry was
# refused. Join broker before returning so failure diagnostics retain no image roots.
# RU: Borrow kind (zero scalar, one result); вернуть признак immediate reentry refusal.
# Join broker до возврата, чтобы failure diagnostics не удерживали image roots.
exercise(kind: int): int
{
    sys := load Sys Sys->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    done := chan[1] of int;
    spawn connect(sys->fildes(0), ready, done);
    (connected, endpoint, variable, function) := <-ready;
    if (connected != 0) {
        raise "terminal publication greeting";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    status, unused: int;
    if (kind == 0) {
        (status, unused) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    } else {
        image: Proxynet->Image;
        (status, image) = net->fetch(endpoint, function, variable, variable, 2, 1, Proxyhost->Variable, 10000);
        if (image.lease != nil) {
            raise "terminal publication adopted corrupt result";
        }
    }
    if (status != Proxyowner->Unknown) {
        raise "terminal publication trusted corrupt reply";
    }
    (status, unused) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    refused := status == Proxynet->Disconnected;
    if (<-done != 1 || endpoint.state.closed != Proxynet->Closed) {
        raise "terminal publication did not settle closed";
    }
    return refused;
}


# EN/RU: Fail only after exercise roots retire; original bytecode selects scalar.
# Отказать лишь после retirement exercise roots; original bytecode выбирает scalar.
init()
{
    if (!exercise(0)) {
        raise "terminal publication exposed reply before refusing reentry";
    }
    sys := load Sys Sys->PATH;
    sys->fprint(sys->fildes(2), "PASS terminal publication: immediate next call refused before settled closure\n");
}
