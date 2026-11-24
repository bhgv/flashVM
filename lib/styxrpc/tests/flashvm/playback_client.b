# Copyright (c) 2026 bhgv
# EN: Drive an actual FlashVM SWF through two ordinary Styx files from DisVM.
# RU: Управление настоящим SWF во FlashVM двумя обычными файлами Styx из DisVM.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys;
state, upload: string;

# EN: Check the state line and transitions without depending on telemetry order.
# RU: Проверить строку состояния и переходы без зависимости от порядка статистики.
expect(value, prefix: string)
{
    if (len value < len prefix || value[0:len prefix] != prefix) {
        raise sys->sprint("state mismatch: expected %s, read %s", prefix, value);
    }
}

# EN: Read one possibly partial byte slice from the exact state fid.
# RU: Прочитать один, возможно короткий, фрагмент с точного fid состояния.
readstate(fd: ref Sys->FD, count: int): string
{
    data := array[count] of byte;
    n := sys->read(fd, data, count);
    if (n < 0) {
        raise sys->sprint("state read: %r");
    }
    return string data[0:n];
}

# EN: Open an independent snapshot for a new measurement.
# RU: Открыть независимый снимок для нового измерения.
getstate(): string
{
    return readstate(sys->open(state, Sys->OREAD), 4096);
}

# EN: Send a counted command at offset zero, checking its expected acknowledgement.
# RU: Передать команду с длиной со смещения ноль и проверить ожидаемый ответ.
send(word: string, succeeds: int)
{
    fd := sys->open(state, Sys->OWRITE);
    bytes := array of byte word;
    written := sys->write(fd, bytes, len bytes);
    if ((written == len bytes) != succeeds) {
        raise sys->sprint("command %s: %d %r", word, written);
    }
}

# EN: Locate a reported key in the snapshot and return its text value.
# RU: Найти ключ в снимке и вернуть его текстовое значение.
metric(snapshot, key: string): string
{
    (unused, lines) := sys->tokenize(snapshot, "\n");
    for (; lines != nil; lines = tl lines) {
        line := hd lines;
        if (len line > len key && line[0:len key] == key) {
            return line[len key:];
        }
    }
    raise "missing playback metric " + key;
}


# EN: Upload an AVM1 frame and exercise each lifecycle transition over a real mount.
# RU: Загрузить кадр AVM1 и провести каждый переход через настоящее монтирование.
init()
{
    sys = load Sys Sys->PATH;
    state = "/rpc/.state";
    upload = "/rpc/.upload";
    old := sys->open(state, Sys->OREAD);
    expect(readstate(old, 6), "state=");
    expect(getstate(), "state=empty\n");
    send("start", 0);
    # EN/RU: SWF8 DoAction: set boot = 7 in the first frame / присвоить boot = 7 в первом кадре.
    swf := array[] of {byte 0, byte 0, byte 0, byte 36,
        byte 70, byte 87, byte 83, byte 8, byte 36, byte 0, byte 0, byte 0,
        byte 8, byte 0, byte 0, byte 12, byte 1, byte 0,
        byte 16, byte 3, byte 150, byte 11, byte 0, byte 0, byte 98, byte 111, byte 111, byte 116, byte 0,
        byte 7, byte 7, byte 0, byte 0, byte 0, byte 29, byte 0, byte 64, byte 0, byte 0, byte 0};
    fd := sys->open(upload, Sys->OWRITE);
    if (fd == nil || sys->write(fd, swf, 3) != 3 || sys->open(upload, Sys->OWRITE) != nil) {
        raise "partial or concurrent upload";
    }
    fd = nil;
    expect(getstate(), "state=empty\n");
    fd = sys->open(upload, Sys->OWRITE);
    if (fd == nil || sys->write(fd, swf, 2) != 2 ||
        sys->write(fd, swf[2:], len swf - 2) != len swf - 2) {
        raise sys->sprint("fragmented upload: %r");
    }
    fd = nil;
    expect(getstate(), "state=ready\nlast_result=0\nticks=0\n");
    expect(readstate(old, 4096), "empty\n");
    send("start\n", 1);
    running := getstate();
    expect(running, "state=running\nlast_result=0\nticks=1\n");
    if (int metric(running, "swf_bytes=") != 36 || int metric(running, "live_objects=") <= 0 ||
        int metric(running, "interpreter_entries=") + int metric(running, "native_entries=") <= 0) {
        raise "first frame or VM statistics missing";
    }
    threads := metric(running, "process.threads=");
    if (threads != "unavailable" && (int threads <= 0 || int metric(running, "process.cpu_rows=") <= 0)) {
        raise "process thread counters missing";
    }
    send("pause", 1);
    expect(getstate(), "state=paused\nlast_result=0\nticks=1\n");
    bad := array[] of {byte 0, byte 0, byte 0, byte 1, byte 7};
    fd = sys->open(upload, Sys->OWRITE);
    if (fd == nil || sys->write(fd, bad, len bad) >= 0) {
        raise "invalid SWF was accepted";
    }
    fd = nil;
    expect(getstate(), "state=paused\n");
    send("restart", 1);
    expect(getstate(), "state=running\nlast_result=0\nticks=1\n");
    send("start pause", 0);
    send("debug", 1);
    debugging := getstate();
    expect(debugging, "state=debug\nlast_result=0\nticks=1\n");
    if (metric(debugging, "debug_attached=") != "1") {
        raise "debugger callback did not attach";
    }
    sys->print("PASS DisVM loaded FlashVM SWF controls\n");
}
