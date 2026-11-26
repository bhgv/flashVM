# Copyright (c) 2026 bhgv
# EN: Inspect a failed frame and replace its movie on the same StyxRPC session.
# RU: Прочитать состояние после ошибки кадра и заменить фильм на той же сессии StyxRPC.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys; # File operations on the paired mount.


include "named_movie_upload.b";


# EN: Read the initial status fields from a fresh snapshot on the mounted player.
# RU: Прочитать первые поля свежего снимка состояния смонтированного плеера.
state(): string
{
    fd := sys->open("/peer/.state", Sys->OREAD);
    if (fd == nil) {
        raise sys->sprint("cannot open frame state: %r");
    }
    bytes := array[512] of byte;
    count := sys->read(fd, bytes, len bytes);
    if (count <= 0) {
        raise sys->sprint("cannot read frame state: %r");
    }
    return string bytes[0:count];
}


# EN: Verify failed status, then wait for a new SWF to execute on the same connection.
# RU: Проверить состояние ошибки, затем дождаться выполнения нового SWF на том же соединении.
init()
{
    sys = load Sys Sys->PATH;
    start("peer-ref-failure.swf");
    failed := "state=failed\nlast_result=-3\nticks=0\n";
    for (attempt := 0; attempt < 200; ++attempt) {
        snapshot := state();
        if (len snapshot >= len failed && snapshot[0:len failed] == failed) {
            start("peer-ref-provider.swf");
            for (tick := 0; tick < 200; ++tick) {
                (found, info) := sys->stat("/peer/ref-release");
                if (found == 0) {
                    sys->print("PASS DisVM inspected and replaced a failed FlashVM movie\n");
                    return;
                }
                sys->sleep(10);
            }
            raise "the replacement SWF did not publish its callback";
        }
        sys->sleep(10);
    }
    raise sys->sprint("movie did not report the expected frame error: %s", state());
}
