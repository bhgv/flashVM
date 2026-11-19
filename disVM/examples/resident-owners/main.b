# Copyright (c) 2026 bhgv
# EN: Four permanent native Dis owners exchange copied results through channels.
# RU: Четыре закреплённых нативных владельца Dis передают результаты через каналы.
implement ResidentOwners;
include "sys.m";

ResidentOwners: module {
    init: fn(); # Entry uses ordinary Sys/FD capabilities without a display context.
};


# EN: Calculate lane's sum with private scratch storage, then reply once.
# lane is 1..4; replies is borrowed until the copied (lane,sum) rendezvous completes.
# The calling Dis process stays on its own native thread; no execution pool exists.
# RU: Вычислить сумму для lane с личным scratch, затем ответить один раз.
# lane равен 1..4; replies заимствован до передачи копии (lane,sum).
# Процесс остаётся на своём нативном потоке; пула исполнителей нет.
calculate(lane: int, replies: chan of (int, int))
{
    sys := load Sys Sys->PATH;
    if (sys == nil) {
        raise "fail: resident owner Sys unavailable";
    }
    label := sys->sprint("owner-%d", lane);
    if (sys->werrstr(label) != 0) {
        raise "fail: resident error state";
    }
    scratch := array[256] of int;
    total := 0;
    for (index := 1; index <= 2000; index++) {
        total += lane * index;
        scratch[index % len scratch] = total;
    }
    if (scratch[2000 % len scratch] != total || sys->sprint("%r") != label) {
        raise "fail: resident private state";
    }
    replies <-= (lane, total);
}


# EN: Start four owners, verify independent copied replies and return normally.
# No parameters. The stack roots replies until all children finish their sends;
# errors raise, normal runtime teardown joins every owner. No GUI is required.
# RU: Запустить четыре владельца, проверить независимые ответы и вернуться.
# Аргументов нет. Стек удерживает replies до четырёх ответов; ошибка вызывает
# исключение, runtime присоединяет потоки при завершении. GUI не требуется.
init()
{
    sys := load Sys Sys->PATH;
    if (sys == nil) {
        raise "fail: resident example Sys unavailable";
    }
    replies := chan of (int, int);
    for (lane := 1; lane <= 4; lane++) {
        spawn calculate(lane, replies);
    }
    seen := array[5] of int;
    for (finished := 0; finished < 4; finished++) {
        (lane, total) := <-replies;
        if (lane < 1 || lane > 4) {
            raise "fail: resident lane";
        }
        if (seen[lane] || total != lane * 2001000) {
            raise "fail: resident copied result";
        }
        seen[lane] = 1;
    }
    sys->print("PASS resident owners: children=4 sums=ok private-state=ok channels=ok\n");
}
