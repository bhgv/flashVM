# Copyright (c) 2026 bhgv
# EN: Repeated forwarding imports reuse retired actor slots and actual owner generations.
# RU: Повторные forwarding imports используют slots retired actors и actual owner generations.
implement ProxyRelayChurn;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyRelayChurn: module { init: fn(); };


# EN: Import/use/drop forty independent views on one connection. Source storage
# stays live and every obsolete lease stays revoked across multiple pool wraps.
# RU: Import/use/drop сорока independent views на одной connection. Source storage
# остаётся live, obsolete leases остаются revoked после нескольких pool wraps.
init()
{
    sys := load Sys Sys->PATH;
    net := load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 33333, big 44444, 30000, ready);
    (status, endpoint, local, function) := <-ready;
    if (status != 0) {
        raise "forwarding churn greeting";
    }
    absent := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    previous := absent;
    for (round := 0; round < 40; round++) {
        (imported, image) := net->importref(endpoint, function, 0, 10000);
        if (imported != 0 || image.lease == nil) {
            raise sys->sprint("forwarding churn capacity did not recover: round=%d status=%d", round, imported);
        }
        if (round > 0) {
            (stale, unused) := net->call(endpoint, Proxyowner->Read, previous, absent, absent, 0, 0, 10000);
            if (stale != Proxynet->Stale || (image.owner.session == previous.owner.session && image.owner.slot == previous.owner.slot &&
                image.owner.generation == previous.owner.generation)) {
                raise "forwarding churn revived consumed lease or native generation";
            }
        }
        (updated, value) := net->call(endpoint, Proxyowner->Add, image, absent, absent, 0, 1, 10000);
        if (updated != 0 || value != 11 + round) {
            raise "forwarding churn lost original source value";
        }
        (dropped, unused) := net->call(endpoint, Proxyowner->Drop, image, absent, absent, 0, 0, 10000);
        if (dropped != 0) {
            raise "forwarding churn Drop";
        }
        previous = image;
        image = absent;
    }
    previous = absent;
    (read, value) := net->call(endpoint, Proxyowner->Read, local, absent, absent, 0, 0, 10000);
    if (read != 0 || value != 0) {
        raise "forwarding churn changed unrelated local storage";
    }
    (stopped, unused) := net->call(endpoint, Proxyowner->Stop, absent, absent, absent, 0, 0, 10000);
    if (stopped != 0) {
        raise "forwarding churn terminal Stop";
    }
    sys->fprint(sys->fildes(2), "PASS forwarding churn: 40 imports original storage stale leases reusable actor pool\n");
}
