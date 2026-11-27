# Copyright (c) 2026 bhgv
# EN: Start the source-built AS2 movie and check its completed typed value exchange.
# RU: Запустить собранный фильм AS2 и проверить завершение типизированного обмена.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys; # Local file checks, movie transport and monotonic clock.
BudgetMs: con 5000; # Limit completion of the three direct calls.

include "named_movie_upload.b";


# EN: Upload the movie to the paired player and wait for its ordinary VFS marker.
# RU: Загрузить фильм в плеер пары и дождаться его метки в обычной VFS.
init()
{
    sys = load Sys Sys->PATH;
    start("live-values.swf");
    began := sys->millisec();
    while (sys->millisec() - began < BudgetMs) {
        (ok, nil) := sys->stat("/wide-done");
        if (ok == 0) {
            sys->print("PASS native live-values RPC paired VMs\n");
            return;
        }
        sys->sleep(5);
    }
    raise "missing copied array, ref copy-back or live channel acknowledgement";
}
