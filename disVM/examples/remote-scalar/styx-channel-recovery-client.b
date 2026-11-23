# Copyright (c) 2026 bhgv
# EN: Check channel negotiation, diagnostic snapshots and transport-loss outcomes.
# RU: Проверить профиль каналов, диагностические снимки и последствия обрыва.
implement Recovery;
include "sys.m";
include "processctl.m";
include "chain-channel-lifetime.m";

Recovery: module { init: fn(); };

Modulepath: con "/transport/mounted/channel-lifetime.dis";


# EN: Read one snapshot in small chunks until EOF on the same open fid.
# RU: Прочитать снимок небольшими частями до EOF на одном открытом fid.
snapshot(sys: Sys): string
{
    file := sys->open(Modulepath + "/.unibus-status", Sys->OREAD);
    if (file == nil) {
        raise sys->sprint("status open: %r");
    }
    bytes := array[32] of byte;
    text := "";
    for (;;) {
        n := sys->read(file, bytes, len bytes);
        if (n < 0) {
            raise sys->sprint("status read: %r");
        }
        if (n == 0) {
            return text;
        }
        text += string bytes[0:n];
    }
}


# EN: Find an exact metadata field in a space-separated snapshot.
# RU: Найти точное поле метаданных в снимке с пробельными разделителями.
field(sys: Sys, text, expected: string): int
{
    (nil, entries) := sys->tokenize(text, " \n");
    for (; entries != nil; entries = tl entries) {
        if (hd entries == expected) {
            return 1;
        }
    }
    return 0;
}


# EN: Verify the occupied tag count stays within the known fixture capacity.
# RU: Проверить, что число занятых тегов укладывается в ёмкость примера.
tagcounts(sys: Sys, text: string): int
{
    for (used := 0; used <= 8; used++) {
        if (field(sys, text, sys->sprint("reverse_styx_tags=%d/8", used))) {
            return 1;
        }
    }
    return 0;
}


# EN: Find one row for the exact caller and state, and require a wire fid.
# RU: Найти строку точного клиента и состояния и проверить наличие номера fid.
callrow(sys: Sys, text: string, pid: int, state: string): int
{
    (nil, rows) := sys->tokenize(text, "\n");
    for (; rows != nil; rows = tl rows) {
        row := hd rows;
        if (len row >= 4 && row[0:4] == "fid=" &&
            field(sys, row, sys->sprint("pid=%d", pid)) && field(sys, row, "state=" + state)) {
            return 1;
        }
    }
    return 0;
}


# EN: Announce the verified wait to the host, which shuts down the live socket.
# RU: Сообщить хосту о проверенном ожидании, после чего он разорвёт сокет.
arm(sys: Sys)
{
    fd := sys->create("break-now", Sys->OWRITE, 8r666);
    if (fd == nil) {
        raise sys->sprint("cannot arm link break: %r");
    }
}


# EN: Wait for the actual remote entry marker, then verify its PID in the snapshot.
# RU: Дождаться метки входа провайдера, затем сверить PID вызова в снимке.
watch_call(sys: Sys, pid: int, entered: chan of int)
{
    if (<-entered != 1) {
        raise "wrong provider entry marker";
    }
    state := snapshot(sys);
    if (!tagcounts(sys, state) || !callrow(sys, state, pid, "pending")) {
        raise "missing pending function fid in diagnostic snapshot";
    }
    arm(sys);
}


# EN: Observe a native receive wait by lifetime ID before requesting the link break.
# RU: Найти нативное ожидание приёма по ID времени жизни перед разрывом связи.
watch_receive(sys: Sys, ctl: Processctl, pids: chan of int)
{
    pid := <-pids;
    cursor := big 0;
    for (;;) {
        (status, id, found, nil) := ctl->resident(cursor);
        if (status != 1 || id <= cursor) {
            raise "receiver not registered";
        }
        cursor = id;
        if (found == pid) {
            break;
        }
    }
    for (i := 0; i < 3000; i++) {
        (status, activity) := ctl->residentactivity(cursor);
        if (status == 1 && activity == Processctl->NativeActivity) {
            arm(sys);
            return;
        }
        sys->sleep(1);
    }
    raise "receiver did not enter native wait";
}


# EN: Start a native receive in a resident registered with Processctl.
# RU: Начать нативный приём в потоке, зарегистрированном в Processctl.
receive_until_break(ctl: Processctl, port, pids: chan of int, done: chan of string)
{
    pids <-= ctl->pid();
    problem := "";
    {
        value := <-port;
    } exception detail {
        "remote channel operation failed" => problem = "remote channel operation failed";
        * => raise detail;
    }
    done <-= problem;
}


# EN: Select one scenario from a local fixture file and report only verified outcomes.
# RU: Выбрать сценарий из локального файла и сообщить о проверенных результатах.
init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    config := sys->open("case", Sys->OREAD);
    bytes := array[16] of byte;
    count := sys->read(config, bytes, len bytes);
    mode := string bytes[0:count];
    api := load Chainchannellifetime Modulepath;
    if (mode == "legacy") {
        if (api != nil || sys->sprint("%r") != "incompatible Styx UniBus channel profile") {
            raise sys->sprint("expected incompatible channel profile: %r");
        }
        sys->print("PASS incompatible Styx profile at load\n");
        return;
    }
    if (api == nil) {
        raise sys->sprint("cannot load recovery fixture: %r");
    }
    if (mode == "call") {
        waiting := chan of int;
        entered := chan of int;
        spawn watch_call(sys, ctl->pid(), entered);
        {
            value := api->waitvalue(waiting, entered);
            raise "call returned despite transport loss";
        } exception detail {
            "remote managed call outcome unknown" => sys->print("PASS unknown call outcome\n");
            * => raise detail;
        }
        return;
    }
    (port, alias) := api->buffered(109);
    if (port == nil || alias != port || <-port != 109 ||
        !callrow(sys, snapshot(sys), ctl->pid(), "published")) {
        raise "failed to read live channel and lease snapshot";
    }
    if (mode == "fresh") {
        sys->print("PASS fresh mount with working channel\n");
        return;
    }
    pids := chan of int;
    done := chan of string;
    spawn receive_until_break(ctl, port, pids, done);
    watch_receive(sys, ctl, pids);
    problem := <-done;
    if (problem != "remote channel operation failed") {
        raise "unexpected channel wait outcome " + problem;
    }
    sys->print("PASS interrupted channel wait\n");
}
