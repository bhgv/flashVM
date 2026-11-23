# Copyright (c) 2026 bhgv
# EN: Verify which branch commits for blocking mixed selections over TCP.
# RU: Проверять выбранную ветку блокирующего смешанного ALT через TCP.
implement Mixedclient;
include "sys.m";
include "processctl.m";
include "chain-channel-lifetime.m";

Mixedclient: module { init: fn(); };


# EN: Wait for provider entry, then supply the delayed remote branch.
# RU: Дождаться входа провайдера и предоставить значение удалённой ветке с задержкой.
delayed_remote(sys: Sys, entered, remote: chan of int)
{
    if (<-entered != 19) {
        raise "wrong remote alt entry marker";
    }
    sys->sleep(250);
    remote <-= 74;
}


# EN: Call a mixed ALT with two empty channels and report whether cancellation won.
# RU: Вызвать смешанный ALT с двумя пустыми каналами и сообщить об отмене.
wait_cancel(api: Chainchannellifetime, ctl: Processctl, remote, entered, started, done: chan of int)
{
    started <-= ctl->pid();
    cancelled := 0;
    {
        api->mixed(4, remote, entered);
    } exception {
        * => cancelled = 1;
    }
    done <-= cancelled;
}


# EN: Compare the received branch and value to the expected pair.
# RU: Сравнить полученную ветку и значение с ожидаемой парой.
check(result: (int, int), guard, value: int): int
{
    (selected, received) := result;
    return selected == guard && received == value;
}


# EN: Arm both guards. Verify the winning value and that its rival was not consumed.
# RU: Подготовить обе ветки. Проверить выбранное значение и сохранение второго.
check_race(api: Chainchannellifetime, remote: chan of int)
{
    remote <-= 85;
    (guard, value) := api->mixed(5, remote, nil);
    if (guard == 1 && value == 71) {
        alt {
            untouched := <-remote =>
                if (untouched != 85) {
                    raise "mixed ALT changed the unselected remote payload";
                }
            * => raise "mixed ALT consumed both branches";
        }
    } else if (guard != 2 || value != 85) {
        raise "mixed ALT chose an invalid branch or payload";
    }
}


# EN: Check ready and delayed local wins, a remote win, same-device mixed rendezvous,
# and cancellation after the provider enters its function. Fail on any inconsistent branch.
# RU: Проверить готовую и отложенную локальные ветки, удалённую ветку, встречный ALT
# на одном провайдере и отмену после его входа в функцию. При расхождении бросить ошибку.
init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    api := load Chainchannellifetime "/transport/mounted/channel-lifetime.dis";
    if (api == nil) {
        raise sys->sprint("cannot load mixed channel fixture: %r");
    }
    remote := chan of int;
    if (!check(api->mixed(0, remote, nil), 1, 71) ||
        !check(api->mixed(1, remote, nil), 1, 72) ||
        !check(api->mixed(3, remote, nil), 1, 73)) {
        raise "mixed ALT lost its local guard";
    }
    entered := chan of int;
    spawn delayed_remote(sys, entered, remote);
    if (!check(api->mixed(2, remote, entered), 2, 74)) {
        raise "mixed ALT lost its delayed remote guard";
    }
    if (!check(api->mixed(6, remote, nil), 2, 76)) {
        raise "provider mixed ALTs could not pair on the caller's channel";
    }
    buffered := chan[1] of int;
    started := chan of int;
    done := chan of int;
    spawn wait_cancel(api, ctl, buffered, entered, started, done);
    pid := <-started;
    if (<-entered != 19) {
        raise "cancellable mixed ALT did not enter";
    }
    sys->sleep(250);
    if (ctl->cancelwait(pid) != Processctl->Cancelled || <-done != 1) {
        raise "mixed ALT cancellation did not retire the waiter";
    }
    buffered <-= 75;
    if (!check(api->mixed(2, buffered, nil), 2, 75)) {
        raise "cancelled mixed ALT affected the next call";
    }
    for (i := 0; i < 8; i++) {
        check_race(api, buffered);
    }
    sys->print("PASS blocking mixed channel ALT\n");
}
