# Copyright (c) 2026 bhgv
# EN: Verify provider-owned returned channel identity, lifetime and teardown.
# RU: Проверять идентичность, время жизни и освобождение возвращённых каналов провайдера.
implement Resultclient;
include "sys.m";
include "processctl.m";
include "chain-channel-lifetime.m";

Resultclient: module { init: fn(); };


# EN: Attempt a blocking receive and publish the calling PID and cancellation result.
# RU: Выполнить блокирующий приём, передать PID вызывающего и исход отмены.
wait_cancel(ctl: Processctl, port, started, done: chan of int)
{
    started <-= ctl->pid();
    cancelled := 0;
    {
        value := <-port;
    } exception {
        * => cancelled = 1;
    }
    done <-= cancelled;
}


# EN: Wait until the child has entered its cancellable Styx operation.
# RU: Дождаться входа дочернего потока в отменяемую операцию Styx.
wait_native(sys: Sys, ctl: Processctl, pid: int)
{
    cursor := big 0;
    for (;;) {
        (status, id, found, retired) := ctl->resident(cursor);
        if (status != 1 || id <= cursor) {
            raise "returned channel receiver left the resident registry";
        }
        cursor = id;
        if (found == pid) {
            break;
        }
    }
    for (attempt := 0; attempt < 1000; attempt++) {
        (status, activity) := ctl->residentactivity(cursor);
        if (status != 1) {
            raise "returned channel receiver retired before cancellation";
        }
        if (activity == Processctl->NativeActivity) {
            return;
        }
        sys->sleep(1);
    }
    raise "returned channel receiver did not start its native wait";
}


# EN: Consume one returned buffered channel and release both its references on return.
# RU: Прочитать буферизованный канал и освободить обе ссылки при выходе.
retire(api: Chainchannellifetime, expected: int)
{
    (port, alias) := api->buffered(expected);
    if (port != alias || <-port != expected) {
        raise "fresh buffered route changed identity while recycling";
    }
}


# EN: Send on the second route after the first route has been offered.
# RU: Отправить во второй маршрут после начала ожидания на первом.
delayed_send(sys: Sys, port: chan of int)
{
    sys->sleep(200);
    port <-= 79;
}


# EN: Exercise late-bound nested keys, exact aliases, and returning keys to their owner.
# RU: Проверить поздние вложенные ключи, идентичность и возврат ключей владельцу.
check_nested(api: Chainchannellifetime)
{
    (outer, original) := api->nested();
    (known, fresh) := <-outer;
    (alias, again) := <-outer;
    if (known != original || again != original || fresh != alias ||
        original == fresh || <-original != 83 || <-fresh != 87) {
        raise "nested returned channel changed identity or payload";
    }
    outer <-= (alias, original);
    (sent, known_again) := <-outer;
    if (sent != fresh || known_again != original) {
        raise "nested channel keys changed on round trip";
    }
}


# EN: Run multiple returned routes concurrently and cancel just one operation fid.
# RU: Удерживать несколько возвращённых маршрутов и отменить один fid операции.
init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    api := load Chainchannellifetime "/transport/mounted/channel-lifetime.dis";
    if (api == nil) {
        raise sys->sprint("cannot load returned channel fixture: %r");
    }
    (first, alias) := api->buffered(31);
    (second, same) := api->buffered(47);
    if (first == nil || second == nil || first != alias || second != same ||
        first == second || <-second != 47 || <-alias != 31) {
        raise "returned handles lost scope, identity or buffered contents";
    }
    same <-= 59;
    alt {
        value := <-first =>
            raise "multiple-route ALT chose the empty route";
        value := <-second =>
            if (value != 59) {
                raise "multiple-route ALT lost its immediate payload";
            }
        * => raise "multiple-route ALT overlooked a ready route";
    }
    alt {
        <-first or <-second =>
            raise "multiple-route ALT consumed an empty route";
        * => ;
    }
    spawn delayed_send(sys, second);
    alt {
        value := <-first =>
            raise "blocking multiple-route ALT chose the empty route";
        value := <-second =>
            if (value != 79) {
                raise "blocking multiple-route ALT lost its delayed payload";
            }
    }
    first <-= 81;
    second <-= 82;
    alt {
        value := <-first =>
            if (value != 81 || <-second != 82) {
                raise "multiple-route ALT changed the losing second branch";
            }
        value := <-second =>
            if (value != 82 || <-first != 81) {
                raise "multiple-route ALT changed the losing first branch";
            }
    }
    first <-= 89;
    (old, relayed) := api->relay(first, 97);
    if (old != 89 || relayed != first || <-alias != 97) {
        raise "returned channel failed as a second call argument";
    }
    check_nested(api);
    (forwarded, reply) := api->worker(200);
    api->relayput(forwarded, 13);
    if (<-reply != 213) {
        raise "unbuffered send through a second call missed the original receiver";
    }
    (input, output) := api->worker(100);
    api = nil;
    input <-= 23;
    alt {
        value := <-input =>
            raise "returned ALT chose its empty channel";
        value := <-output =>
            if (value != 123) {
                raise "returned unbuffered channels lost their resident";
            }
    }
    started := chan of int;
    done := chan of int;
    spawn wait_cancel(ctl, alias, started, done);
    pid := <-started;
    wait_native(sys, ctl, pid);
    if (ctl->cancelwait(pid) != Processctl->Cancelled || <-done != 1) {
        raise "returned channel operation did not cancel";
    }
    api = load Chainchannellifetime "/transport/mounted/channel-lifetime.dis";
    if (api == nil) {
        raise sys->sprint("cannot reload while returned channels are live: %r");
    }
    # EN: Cross the provider's 128 live lease slots while older routes stay open.
    # RU: Превысить 128 живых аренд провайдера, пока прежние маршруты ещё открыты.
    for (i := 0; i < 144; i++) {
        retire(api, 500+i);
    }
    same <-= 53;
    first <-= 61;
    if (<-second != 53 || <-alias != 61) {
        raise "retiring other handles or cancelling an operation closed a live route";
    }
    sys->print("PASS provider-owned returned channels\n");
}
