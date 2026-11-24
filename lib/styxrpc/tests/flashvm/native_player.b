# Copyright (c) 2026 bhgv
# EN: Drive the actual paired player, then verify independent frame deadlines.
# RU: Управление штатным парным плеером и проверка самостоятельных кадровых тактов.
implement Client;
include "sys.m";

Client: module { init: fn(); };
Movieview: module {
    key: fn(code: int): int;
    snapshot: fn(unused: int): int;
    label: fn(text: string): (real, string);
};
sys: Sys;


# EN: Take and read a complete independent state snapshot.
# RU: Открыть и прочитать независимый снимок состояния.
state(): string
{
    fd := sys->open("/rpc/.state", Sys->OREAD);
    bytes := array[4096] of byte;
    size := sys->read(fd, bytes, len bytes);
    if (size <= 0) {
        raise sys->sprint("state read: %r");
    }
    return string bytes[0:size];
}


# EN: Look up a counted key including its equals delimiter.
# RU: Найти ключ вместе с разделителем равенства.
field(snapshot, key: string): string
{
    (unused, lines) := sys->tokenize(snapshot, "\n");
    for (; lines != nil; lines = tl lines) {
        line := hd lines;
        if (len line > len key && line[0:len key] == key) {
            return line[len key:];
        }
    }
    raise "missing state field " + key;
}


# EN: Write one command at offset zero.
# RU: Записать одну команду со смещения ноль.
command(word: string)
{
    fd := sys->open("/rpc/.state", Sys->OWRITE);
    data := array of byte word;
    if (fd == nil || sys->write(fd, data, len data) != len data) {
        raise sys->sprint("command %s: %r", word);
    }
}


# EN: Upload the startup movie, using the declared payload extent.
# RU: Загрузить стартовый фильм с объявленной длиной.
upload()
{
    source := sys->open("paired-root.swf", Sys->OREAD);
    bytes := array[4096] of byte;
    size := sys->read(source, bytes[4:], len bytes - 4);
    if (size <= 0 || size >= len bytes - 4) {
        raise "cannot read startup fixture";
    }
    bytes[2] = byte (size >> 8);
    bytes[3] = byte size;
    fd := sys->open("/rpc/.upload", Sys->OWRITE);
    if (fd == nil || sys->write(fd, bytes, size + 4) != size + 4) {
        raise sys->sprint("upload: %r");
    }
}


# EN: Invoke input and read back the live MovieClip position through ordinary typed Dis load.
# RU: Подать ввод и прочитать координату MovieClip через обычный типизированный load Dis.
check_view(initial, code: int): ref Sys->FD
{
    path := "/rpc/mounted/movie-view.dis";
    image := load Movieview path;
    if (image == nil) {
        raise sys->sprint("cannot load SWF view: %r");
    }
    if (image->snapshot(0) != initial || image->key(code) != code || image->snapshot(0) != code) {
        raise "input did not update the SWF movie clip";
    }
    (coordinate, label) := image->label("а\0b");
    if (coordinate != real code || label != "а\0b") {
        raise "managed callback lost the movie state or counted input";
    }
    fid := sys->open(path + "/.interface", Sys->OREAD);
    bytes := array[1] of byte;
    if (fid == nil || sys->read(fid, bytes, 1) != 1) {
        raise "could not keep original image fid";
    }
    return fid;
}


# EN: Remove the layer, then check a second run on the existing Styx connection.
# RU: Удалить слой и проверить повторный запуск в том же соединении Styx.
check_unmounted(old: ref Sys->FD)
{
    if (sys->unmount("/assets", "/movies") < 0) {
        raise "could not remove the live movie binding";
    }
    upload();
    command("start");
    for (attempt := 0; attempt < 100; ++attempt) {
        current := state();
        if (int field(current, "ticks=") >= 3) {
            if (field(current, "movies_loaded=") != "0" ||
                field(current, "movie_load_errors=") != "6") {
                raise "old layer was still visible after unmount";
            }
            bytes := array[1] of byte;
            if (sys->read(old, bytes, 1) >= 0) {
                raise "old fid reached the replacement movie";
            }
            check_view(0, 66);
            return;
        }
        sys->sleep(20);
    }
    raise "player did not run after unmount";
}


# EN: Wait for three automatic ticks and a grandchild load, then confirm pause.
# RU: Дождаться трёх автоматических тактов и вложенного файла, затем подтвердить паузу.
init()
{
    sys = load Sys Sys->PATH;
    if (sys->bind("/assets", "/movies", Sys->MREPL) < 0) {
        raise "could not bind the live movie provider";
    }
    upload();
    if (field(state(), "state=") != "ready") {
        raise "player did not prepare the upload";
    }
    command("start");
    for (attempt := 0; attempt < 100; ++attempt) {
        current := state();
        if (int field(current, "ticks=") >= 3 && field(current, "movies_loaded=") == "2") {
            if (field(current, "movie_load_errors=") != "5") {
                raise "shared-root rejection count differs";
            }
            command("pause");
            frozen := state();
            sys->sleep(150);
            after := state();
            if (field(after, "state=") != "paused" || field(frozen, "ticks=") != field(after, "ticks=")) {
                raise "paused owner advanced a frame";
            }
            old := check_view(0, 65);
            check_unmounted(old);
            sys->print("PASS native FlashVM and DisVM pair\n");
            return;
        }
        sys->sleep(20);
    }
    snapshot := state();
    raise sys->sprint("automatic playback: state=%s ticks=%s loads=%s errors=%s last=%s",
        field(snapshot, "state="), field(snapshot, "ticks="), field(snapshot, "movies_loaded="),
        field(snapshot, "movie_load_errors="), field(snapshot, "last_movie_result="));
}
