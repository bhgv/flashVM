# Copyright (c) 2026 bhgv
# EN: Execute on B and resolve a separate Styx library on C.
# RU: Выполнить программу на B и загрузить библиотеку Styx с C.
implement Owner;
include "sys.m";

Owner: module { init: fn(); };
include "wide.m";
include "managed.m";


# EN: Read A's stdin, call C, then acknowledge via A's separate output files.
# RU: Прочитать stdin A, вызвать C и вывести результаты на разные FD A.
init()
{
    sys := load Sys Sys->PATH;
    data := array[4] of byte;
    input := sys->fildes(0);
    if (input == nil || sys->readn(input, data, len data) != len data) {
        raise "chain input mismatch";
    }
    if (string data == "drop") {
        sys->fprint(sys->fildes(1), "chain accepted before disconnect\n");
        sys->sleep(30000);
        raise "test transport was not disconnected";
    }
    if (string data != "live") {
        raise "chain input mismatch";
    }
    extra := sys->fildes(3);
    if (extra == nil || sys->write(extra, data, 1) >= 0 ||
        sys->readn(extra, data, len data) != len data || string data != "more") {
        raise "chain FD 3 mismatch";
    }
    if (sys->open("/.prog/.self/.fd/3", Sys->OWRITE) != nil) {
        raise "caller FD view widened read-only access";
    }
    alias := sys->open("/.prog/.self/.fd/7", Sys->ORDWR);
    alias2 := sys->fildes(8);
    bytebuf := array[1] of byte;
    if (alias == nil || alias2 == nil || sys->read(alias, bytebuf, 1) != 1 || string bytebuf != "A" ||
        sys->read(alias2, bytebuf, 1) != 1 || string bytebuf != "B") {
        raise "chain FD aliases did not share the read cursor";
    }
    bytebuf[0] = byte 'C';
    if (sys->write(alias, bytebuf, 1) != 1) {
        raise "chain FD alias lost write access";
    }
    if (sys->bind("/remote-libs/mounted", "/deps", Sys->MAFTER) < 0) {
        raise sys->sprint("could not add library union: %r");
    }
    local := load Wide "/deps/wide.dis";
    if (local == nil || local->echo(big 7) != big 17 ||
        sys->bind("/remote-libs/mounted", "/deps", Sys->MBEFORE) < 0) {
        raise "local library precedence failed";
    }
    library := load Wide "/deps/wide.dis";
    if (library == nil || library->echo(big 4294967296) != big 4294967296 ||
        library->echo(big 1) != big 2) {
        raise "chain library call failed";
    }
    other := load Wide "/other-libs/mounted/wide.dis";
    if (other == nil || other->echo(big 1) != big 1 || library->echo(big 1) != big 3) {
        raise "library mount instances shared state";
    }
    marker := sys->open("/.vfs/chain-stage", Sys->OREAD);
    if (marker == nil)
        raise "B cannot see A's VFS";
    mark := array[4] of byte;
    n := sys->read(marker, mark, len mark);
    if (n != 4 || mark[0] != byte 0 || mark[1] != byte 0 || mark[2] != byte 0 || mark[3] != byte 42)
        raise "invalid A marker on B";
    check_managed();
    if (sys->fprint(sys->fildes(1), "chain stdout\n") <= 0 ||
        sys->fprint(sys->fildes(2), "chain stderr\n") <= 0) {
        raise "chain output failed";
    }
}

# EN: Copy back an aliased graph from C before reporting the launch result to A.
# RU: Получить вложенные обновления с C до передачи результата запуска на A.
check_managed()
{
    remote := load Managed "/deps/managed.dis";
    if (remote == nil)
        raise "chain managed load failed";
    Cell, Bundle: import remote;
    original := ref Cell("from B", array[] of {byte 17});
    state := ref Bundle(original, array[] of {original, nil});
    extra := array[32] of ref Cell;
    extra[0] = original;
    extra[31] = ref Cell("untouched", nil);
    (returned, output) := remote->changegraph(state, extra, 3);
    if (returned != original || original.text != "changed after unlink" ||
        original.bytes[0] != byte 17 || state.direct != state.members[0] ||
        state.direct == original || state.direct.text != "fresh" || state.members[1] != nil ||
        extra[0] != original || extra[31].text != "last index" ||
        len output != 3000 || output[2999] != byte 53) {
        raise "chain managed copy-back failed";
    }
}
