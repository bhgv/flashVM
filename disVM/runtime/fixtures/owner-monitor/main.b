# Copyright (c) 2026 bhgv
# EN: Keep a parent observer while eight permanent owners replace their hooks.
# RU: Сохранить наблюдатель родителя при смене hooks восьми постоянных владельцев.
implement OwnerMonitor;
include "sys.m";
OwnerMonitor: module { init: fn(); };
sys: Sys; # Immutable interface before spawning actual residents.


# EN: Enter the real Sys callback and notify done after native checks finish.
# RU: Войти в настоящий Sys callback и известить done после native-проверок.
worker(done: chan of int)
{
    sys->werrstr("resident observer");
    done <-= 1;
}


# EN: Start/join eight resident guests while the controller hook remains active.
# RU: Запустить/дождаться восьми гостей при активном hook контроллера.
init()
{
    sys = load Sys Sys->PATH;
    sys->werrstr("parent observer start");
    done := chan of int;
    for (id := 0; id < 8; id++) {
        spawn worker(done);
    }
    for (id = 0; id < 8; id++) {
        if (<-done != 1) {
            raise "owner monitor completion";
        }
    }
    sys->werrstr("parent observer finish");
    sys->print("PASS owner-local heap monitors owners=8\n");
}
