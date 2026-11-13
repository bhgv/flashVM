# Copyright (c) 2026 bhgv
# EN: Real default-profile guests acknowledge one result each after native sleep.
# RU: Настоящие гости штатного профиля подтверждают результат после native sleep.
implement NativeDefault;
include "sys.m";
NativeDefault: module { init: fn(); };
sys: Sys; # Immutable interface shared before spawn.


# EN: Borrow shared gate/done, retain id locally and report exactly once.
# RU: Заимствовать gate/done, хранить id локально и ответить ровно один раз.
worker(id: int, gate, done: chan of int)
{
    <-gate;
    sys->sleep(5);
    done <-= id;
}


# EN: Start four dedicated owners, release their gates and validate all IDs.
# RU: Запустить четырёх закреплённых владельцев, открыть gate и проверить все ID.
init()
{
    sys = load Sys Sys->PATH;
    gate := chan of int;
    done := chan of int;
    for (id := 1; id <= 4; id++) {
        spawn worker(id, gate, done);
    }
    for (release := 0; release < 4; release++) {
        gate <-= 1;
    }
    seen := array[5] of int;
    for (completed := 0; completed < 4; completed++) {
        id = <-done;
        if (id < 1 || id > 4 || seen[id]) {
            raise "default native guest identity or completion changed";
        }
        seen[id] = 1;
    }
    sys->print("PASS mandatory native default owners=4\n");
}
