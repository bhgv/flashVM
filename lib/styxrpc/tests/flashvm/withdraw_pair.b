# Copyright (c) 2026 bhgv
# EN: Verify publication withdrawal and fresh identity on a live FlashVM mount.
# RU: Проверить отзыв публикации и новый идентификатор в живом монтировании FlashVM.
implement Client;
include "sys.m";
include "styxexec.m";

Client: module { init: fn(); };
sys: Sys; # Runtime filesystem and scheduler.
exec: Styxexec; # Executable node caller on the same mounted session.
Data: con "/peer/menu/data"; # File that is withdrawn and recreated.
Keep: con "/peer/menu/keep"; # Sibling kept open across all changes.
Gone: con "/peer/gone/deep/value"; # File whose implicit ancestors should vanish.

include "named_movie_upload.b";


# EN: Wait for expected visible metadata on the live remote pathname.
# RU: Дождаться ожидаемых метаданных в живом удалённом пространстве имён.
wait_size(path: string, wanted: big): Sys->Dir
{
    for (i := 0; i < 200; ++i) {
        (ok, info) := sys->stat(path);
        if (ok == 0 && info.length == wanted) {
            return info;
        }
        sys->sleep(10);
    }
    raise "expected FlashVM pathname or size never appeared: " + path;
}


# EN: Enumerate the live directory after withdrawal and check the two visible names.
# RU: После отзыва перечислить живой каталог и проверить два видимых имени.
check_listing()
{
    fd := sys->open("/peer/menu", Sys->OREAD);
    if (fd == nil) {
        raise "cannot open the changed directory";
    }
    seen := 0;
    while (1) {
        (count, entries) := sys->dirread(fd);
        if (count < 0) {
            raise "cannot list the changed directory";
        }
        if (!count) {
            break;
        }
        for (i := 0; i < count; ++i) {
            name := entries[i].name;
            bit := 0;
            if (name == "data" && entries[i].length == big 3) {
                bit = 1;
            }
            if (name == "keep") {
                bit = 2;
            }
            if (!bit || (seen & bit)) {
                raise "unexpected or stale entry in changed directory";
            }
            seen |= bit;
        }
    }
    if (seen != 3) {
        raise "changed directory omitted an entry";
    }
}


# EN: Open three original files, request withdrawal and check each old and new fid.
# RU: Открыть три исходных файла, запросить отзыв и проверить все старые и новые fid.
init()
{
    sys = load Sys Sys->PATH;
    exec = load Styxexec Styxexec->PATH;
    if (exec == nil) {
        raise "cannot load Styxexec";
    }
    exec->init();
    start("withdraw.swf");
    original := wait_size(Data, big 1);
    wait_size(Gone, big 0);
    (menuok, menu) := sys->stat("/peer/menu");
    held := sys->open(Data, Sys->ORDWR);
    sibling := sys->open(Keep, Sys->OREAD);
    retired := sys->open(Gone, Sys->OREAD);
    bytes := array[1] of byte;
    if (menuok < 0 || held == nil || sibling == nil || retired == nil ||
        sys->read(held, bytes, 1) != 1 || bytes[0] != byte 17 ||
        sys->read(sibling, bytes, 1) != 1 || bytes[0] != byte 17 ||
        sys->seek(held, big 0, Sys->SEEKSTART) != big 0 ||
        sys->write(held, bytes, 1) != 1) {
        raise "original fids or callback-time withdrawal did not behave as expected";
    }
    replacement := wait_size(Data, big 3);
    (afterok, after) := sys->stat("/peer/menu");
    (goneok, absent) := sys->stat("/peer/gone");
    (scratchok, discarded) := sys->stat("/peer/scratch");
    if (afterok < 0 || after.qid.path != menu.qid.path || replacement.qid.path == original.qid.path ||
        goneok >= 0 || scratchok >= 0 ||
        sys->read(retired, bytes, 1) >= 0 || sys->seek(held, big 0, Sys->SEEKSTART) != big 0 ||
        sys->read(held, bytes, 1) >= 0 ||
        sys->read(sibling, bytes, 1) != 1 || bytes[0] != byte 17) {
        raise "withdrawal changed a surviving identity or allowed access through a stale fid";
    }
    check_listing();
    fresh := sys->open(Data, Sys->OREAD);
    if (fresh == nil || sys->read(fresh, bytes, 1) != 1 || bytes[0] != byte 83 ||
        sys->open(Data, Sys->OWRITE) != nil) {
        raise "fresh open did not select the replacement and its updated permissions";
    }
    reply := exec->call("/peer", "menu/data", bytes);
    if (len reply != 1 || reply[0] != byte 83) {
        raise "execute did not select the current published callback";
    }
    sys->print("PASS native withdraw RPC paired VMs\n");
}
