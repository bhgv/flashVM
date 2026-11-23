# Copyright (c) 2026 bhgv
# EN: Send a native reverse PID through the hosted result and recover the exact channel.
# RU: Передать нативный встречный PID через результат хоста и вернуть тот же канал.
implement Client;
include "sys.m";
include "channel_pid_result.m";

Client: module { init: fn(); };


# EN: Wait for the exact served result to retire before starting another call.
# RU: Дождаться отзыва выданного результата перед следующим вызовом.
waitretired(sys: Sys, api: Pidresult, tick: chan of int)
{
    for (i := 0; i < 50; i++) {
        if (api->retired(tick)) {
            return;
        }
        sys->sleep(10);
    }
    raise "FlashVM still admitted a proxy after its result fid retired";
}


# EN: Offer a value on a native rendezvous from a second resident.
# RU: Отправить значение через нативный rendezvous из второго резидента.
offer(port: chan of int)
{
    port <-= 19;
}


# EN: Verify current-bus children, independent PIDs and exact identities after echo.
# RU: Проверить дочерние каналы текущей шины, отдельные PID и точные алиасы при возврате.
nested(sys: Sys, api: Pidresult, tick: chan of int)
{
    first := chan[2] of array of chan of int;
    second := chan[1] of array of chan of int;
    child := chan[2] of int;
    other := chan[1] of int;
    sync := chan of int;
    child <-= 81;
    first <-= array[] of {child, child, nil, sync};
    second <-= array[] of {other};
    mailbox := api->nestedbox(17);
    mailbox <-= first;
    mailbox <-= first;
    mailbox <-= second;
    spawn offer(sync);
    if (api->drainNested(tick) != 81 || <-mailbox != first || <-child != 43) {
        raise "FlashVM did not preserve the nested PID graph";
    }
    left := <-first;
    right := <-second;
    if (len left != 4 || left[0] != child || left[1] != child || left[2] != nil ||
        left[3] != sync || len right != 1 || right[0] != other) {
        raise "FlashVM mixed distinct PID routes or lost a nested alias";
    }
    mailbox = nil;
    waitretired(sys, api, tick);
}


# EN: Load FlashVM's compiled module and exercise live grants and result retirement.
# RU: Загрузить модуль FlashVM, проверить живые полномочия и отзыв результата.
init()
{
    sys := load Sys Sys->PATH;
    api := load Pidresult "/rpc/mounted/flash-pid-result.dis";
    if (api == nil) {
        raise sys->sprint("cannot load FlashVM PID result: %r");
    }
    tick := chan[1] of int;
    original := chan[2] of int;
    original <-= 81;
    mailbox := api->mailbox(17);
    mailbox <-= original;
    mailbox <-= original;
    if (api->drain(tick) != 81 || <-mailbox != original || <-original != 43) {
        raise "FlashVM lost the Dis reverse PID or changed its identity";
    }
    mailbox = nil;
    waitretired(sys, api, tick);
    nested(sys, api, tick);
    sys->print("PASS DisVM loaded FlashVM PID result mailbox\n");
}
