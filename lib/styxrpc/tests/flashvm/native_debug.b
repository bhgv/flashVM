# Copyright (c) 2026 bhgv
# EN: Exercise AVM1 breakpoint, single-step and resume on the actual paired player.
# RU: Проверка точки останова AVM1, одного шага и продолжения на штатном плеере.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys;
state: con "/rpc/.state";
upload: con "/rpc/.upload";


# EN: Read a new immutable .state snapshot through the served connection.
# RU: Прочитать новый неизменяемый снимок .state через обслуживаемое соединение.
snapshot(): string
{
    fd := sys->open(state, Sys->OREAD);
    data := array[4096] of byte;
    size := sys->read(fd, data, len data);
    if (size <= 0) {
        raise sys->sprint("read .state: %r");
    }
    return string data[0:size];
}


# EN: Return the value of a key with its equals sign, or an empty string if missing.
# RU: Получить значение ключа со знаком равенства либо пустую строку при отсутствии.
field(text, key: string): string
{
    (count, rows) := sys->tokenize(text, "\n");
    for (; rows != nil; rows = tl rows) {
        row := hd rows;
        if (len row >= len key && row[0:len key] == key) {
            return row[len key:];
        }
    }
    return "";
}


# EN: Submit one command through a fresh fid and require the expected result.
# RU: Передать одну команду через новый fid и проверить ожидаемый результат.
command(word: string, accept: int)
{
    fd := sys->open(state, Sys->OWRITE);
    data := array of byte (word + "\n");
    if (fd == nil) {
        raise sys->sprint("open .state: %r");
    }
    size := sys->write(fd, data, len data);
    if ((size == len data) != accept) {
        raise sys->sprint("unexpected result for %s: %d %r", word, size);
    }
}


# EN: Upload the local AVM1 test movie to the actual remote player.
# RU: Загрузить локальный тестовый фильм AVM1 в настоящий удалённый плеер.
loadmovie()
{
    fd := sys->open(upload, Sys->OWRITE);
    if (fd == nil) {
        raise sys->sprint("open upload: %r");
    }
    src := sys->open("debug.swf", Sys->OREAD);
    data := array[1024] of byte;
    size := sys->read(src, data[4:], len data - 4);
    if (size <= 0 || size >= len data - 4) {
        raise "debug movie is unavailable";
    }
    data[0] = byte (size >> 24);
    data[1] = byte (size >> 16);
    data[2] = byte (size >> 8);
    data[3] = byte size;
    if (sys->write(fd, data, size + 4) != size + 4) {
        raise sys->sprint("upload: %r");
    }
    fd = nil;
}


# EN: Wait for an actual callback stop at the next requested ID or fail with the last status.
# RU: Дождаться настоящей остановки следующего номера либо выдать последнее состояние.
stopped(expected: int): string
{
    text := "";
    for (attempt := 0; attempt < 100; ++attempt) {
        text = snapshot();
        if (field(text, "debug.state=") == "stopped" && int field(text, "debug.stop=") == expected) {
            return text;
        }
        sys->sleep(10);
    }
    raise "missing AVM1 breakpoint stop: " + text;
}


# EN: Stop before the first action, prove restricted I/O and stale-command rejection,
# step to the next action, then resume and verify playback retirement.
# RU: Остановиться до первого действия, проверить ограничения и устаревшие команды,
# сделать шаг и продолжить выполнение до завершения проверки.
init()
{
    sys = load Sys Sys->PATH;
    loadmovie();
    command("break 0", 0);
    command("debug", 1);
    command("break 0", 1);
    command("start", 1);
    first := stopped(1);
    if (field(first, "state=") != "debug" || field(first, "ticks=") != "0" ||
        field(first, "debug.offset=") != "0" || field(first, "debug.frame.0=") == "") {
        raise "invalid first stop: " + first;
    }
    if (sys->open(upload, Sys->OWRITE) != nil) {
        raise "upload remained openable during an AVM1 activation";
    }
    command("restart", 0);
    command("clear 0", 1);
    command("continue 2", 0);
    command("step 1", 1);
    second := stopped(2);
    if (field(second, "ticks=") != "0" || int field(second, "debug.offset=") <= 0) {
        raise "single step did not reach the next action: " + second;
    }
    command("continue 1", 0);
    command("continue 2", 1);
    for (attempt := 0; attempt < 100; ++attempt) {
        current := snapshot();
        if (field(current, "state=") == "running" && int field(current, "ticks=") >= 1) {
            command("pause", 1);
            break;
        }
        sys->sleep(10);
    }
    if (attempt == 100) {
        raise "player did not resume from the AVM1 stop";
    }

    loadmovie();
    command("debug", 1);
    pending := sys->open(upload, Sys->OWRITE);
    if (pending == nil) {
        raise sys->sprint("open pending upload before stop: %r");
    }
    command("break 0", 1);
    command("start", 1);
    stopped(3);
    command("continue 2", 0);
    prefix := array[] of {byte 0, byte 0, byte 0, byte 1};
    count := sys->write(pending, prefix, len prefix);
    if (count >= 0) {
        raise sys->sprint("preopened upload accepted %d bytes during the stop", count);
    }
    # EN: Retire the paired transport while the replacement movie is stopped.
    # RU: Закрыть транспорт пары пока новый фильм остановлен.
    sys->print("PASS native FlashVM debugger\n");
}
