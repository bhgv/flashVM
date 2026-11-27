# Copyright (c) 2026 bhgv
# EN: Retain a real DisVM cell while a FlashVM movie cancels temporary cell requests.
# RU: Удерживать ячейку DisVM, пока фильм FlashVM отменяет временные запросы.
implement Client;
include "sys.m";
include "styxnamed.m";

Client: module { init: fn(); };
sys: Sys; # Real local namespace and acceptance clock.
named: Styxnamed; # Local protocol client for the same named registry.
Port, Cell: import named;
BudgetMs: con 9000; # Bound recovery after the rendezvous stages.
PollMs: con 5; # Recovery marker poll interval.

include "named_movie_upload.b";


# EN: Keep the issuing lease, forward its token at every stage, and inspect the final value.
# RU: Удерживать выдавшую аренду, передавать жетон на каждом этапе и проверить итог.
init()
{
    sys = load Sys Sys->PATH;
    named = load Styxnamed Styxnamed->PATH;
    if (named == nil) {
        raise "cannot load shared cell support";
    }
    named->init();
    original := named->share(array[] of {byte Styxnamed->Number, byte 16r40, byte 16r31,
        byte 0, byte 0, byte 0, byte 0, byte 0, byte 0});
    start("cancel-cell-setup.swf");
    for (stage := 0; stage <= 5; stage++) {
        port := named->open("/cell-" + string stage);
        port.send(original.payload());
        port.close();
    }
    began := sys->millisec();
    while (sys->millisec()-began < BudgetMs) {
        (done, nil) := sys->stat("/control-done");
        if (done == 0) {
            expected := array[] of {byte Styxnamed->Number, byte 16r40, byte 16r52,
                byte 16r40, byte 0, byte 0, byte 0, byte 0, byte 0};
            actual := original.get();
            if (len actual != len expected) {
                raise "the recovery write did not reach the original lease";
            }
            for (i := 0; i < len expected; i++) {
                if (actual[i] != expected[i]) {
                    raise "the recovery write replaced the original identity";
                }
            }
            original.close();
            sys->print("PASS native cancel-cell-setup RPC paired VMs\n");
            return;
        }
        sys->sleep(PollMs);
    }
    raise "missing shared cell recovery marker";
}
