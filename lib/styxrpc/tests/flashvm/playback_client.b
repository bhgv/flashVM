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
    fd := sys->open(state, Sys->OREAD);
    if (fd == nil) {
        raise sys->sprint("cannot open playback state: %r");
    }
    snapshot := "";
    for (;;) {
        part := readstate(fd, 4096);
        if (part == "") {
            return snapshot;
        }
        snapshot += part;
    }
}

# EN: Allow deferred Dis fid retirement before reopening the single upload slot.
# RU: Дождаться отложенного закрытия fid Dis перед повторным открытием слота загрузки.
nextupload(): ref Sys->FD
{
    for (attempt := 0; attempt < 200; ++attempt) {
        fd := sys->open(upload, Sys->OWRITE);
        if (fd != nil) {
            return fd;
        }
        sys->sleep(1);
    }
    raise sys->sprint("could not acquire upload slot: %r");
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


# EN: Verify real child and grandchild execution plus local, malformed and private rejections.
# RU: Проверить выполнение двух вложенных SWF и отклонение локального, битого и закрытого файла.
paired(source: ref Sys->FD)
{
    wire := array[4096] of byte;
    size := sys->read(source, wire[4:], len wire - 4);
    if (size <= 0 || size >= len wire - 4) {
        raise "could not read paired root fixture";
    }
    wire[2] = byte (size >> 8);
    wire[3] = byte size;
    target := sys->open(upload, Sys->OWRITE);
    if (target == nil) {
        raise sys->sprint("cannot open fixture root upload: %r");
    }
    offset := 0;
    while (offset < size+4) {
        written := sys->write(target, wire[offset:], size+4-offset);
        if (written <= 0) {
            raise sys->sprint("cannot upload fixture root at %d: %r", offset);
        }
        offset += written;
    }
    target = nil;
    send("start", 1);
    snapshot := getstate();
    if (metric(snapshot, "movies_loaded=") != "1" || metric(snapshot, "movie_load_errors=") != "4" ||
        metric(snapshot, "child_traces=") != "0") {
        raise "first frame did not stage exactly one valid remote movie: " + snapshot;
    }
    send("pause", 1);
    send("start", 1);
    snapshot = getstate();
    if (metric(snapshot, "movies_loaded=") != "2" || metric(snapshot, "child_traces=") != "1" ||
        metric(snapshot, "leaf_traces=") != "0") {
        raise sys->sprint("child provenance: loads=%s errors=%s last=%s child=%s leaf=%s",
            metric(snapshot, "movies_loaded="), metric(snapshot, "movie_load_errors="),
            metric(snapshot, "last_movie_result="), metric(snapshot, "child_traces="), metric(snapshot, "leaf_traces="));
    }
    send("pause", 1);
    send("start", 1);
    snapshot = getstate();
    if (metric(snapshot, "leaf_traces=") != "1" || metric(snapshot, "other_traces=") != "0" ||
        metric(snapshot, "movie_load_errors=") != "4") {
        raise "grandchild execution or fallback is incorrect: " + snapshot;
    }
    send("debug", 1);
    sys->print("PASS DisVM loaded FlashVM nested SWFs through Styx VFS\n");
}


# EN: Upload an AVM1 frame and exercise each lifecycle transition over a real mount.
# RU: Загрузить кадр AVM1 и провести каждый переход через настоящее монтирование.
init()
{
    sys = load Sys Sys->PATH;
    state = "/rpc/.state";
    upload = "/rpc/.upload";
    fixture := sys->open("paired-root.swf", Sys->OREAD);
    if (fixture != nil) {
        paired(fixture);
        return;
    }
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
    fd = nextupload();
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
    fd = nextupload();
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
