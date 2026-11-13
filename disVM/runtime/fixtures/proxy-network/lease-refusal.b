# Copyright (c) 2026 bhgv
# EN: Partial native image construction preserves outputs and cleans roots on refusal.
# RU: Частичное native image construction сохраняет outputs и очищает roots при отказе.
implement ProxyLeaseRefusal;
include "sys.m";
include "proxygc.m";
ProxyLeaseRefusal: module { init: fn(); };


# EN/RU: The private injected constructor failure leaves nil output and permits retry.
# Private injected constructor failure оставляет nil output и разрешает retry.
init()
{
    sys := load Sys Sys->PATH;
    gc := load Proxygc Proxygc->PATH;
    connection: ref Proxygc->Connection;
    caught := 0;
    {
        connection = gc->open(2);
    } exception {
        "proxy image publication refusal" => caught = 1;
    }
    if (!caught || connection != nil) {
        raise "failed native image construction changed mapped output";
    }
    connection = gc->open(2);
    lease := gc->make(connection, big 1, 0, big 1, 0);
    if (connection == nil || lease == nil || gc->active(lease)) {
        raise "native image construction did not recover";
    }
    gc->close(connection);
    sys->fprint(sys->fildes(2), "PASS native image construction refusal rooted and cleaned\n");
}
