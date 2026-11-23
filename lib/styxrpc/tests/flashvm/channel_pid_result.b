# Copyright (c) 2026 bhgv
# EN: Send a native reverse PID through the hosted result and recover the exact channel.
# RU: Передать нативный встречный PID через результат хоста и вернуть тот же канал.
implement Client;
include "sys.m";
include "channel_pid_result.m";

Client: module { init: fn(); };


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
    for (i := 0; i < 50; i++) {
        if (api->retired(tick)) {
            sys->print("PASS DisVM loaded FlashVM PID result mailbox\n");
            return;
        }
        sys->sleep(10);
    }
    raise "FlashVM still admitted a proxy after its result fid retired";
}
