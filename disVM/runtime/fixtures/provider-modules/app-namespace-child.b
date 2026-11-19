# Copyright (c) 2026 bhgv
# EN: Real bound-module children share the effective namespace until the caller explicitly forks its own group.
# RU: Дети привязанного модуля разделяют выбранное окружение до явного отделения группы вызывающим потоком.
implement Value;
include "value.m";
include "sys.m";
include "processctl.m";
Mode: con TEST_MODE; # 0 shared; 1 child detaches; 2 parent detaches; 3 one failed constructor then recovery.


# EN/RU: Read the real provider data through a selected alias; borrowed Sys module/path, return 1/0.
# RU: Прочитать данные провайдера через псевдоним; Sys и путь заимствуются, вернуть 1/0.
visible(sys: Sys, name: string): int
{
    fd := sys->open(name, Sys->OREAD);
    if (fd == nil) {
        return 0;
    }
    bytes := array[32] of byte;
    n := sys->read(fd, bytes, len bytes);
    return n == 9 && string bytes[:n] == "execution";
}


# EN/RU: Inspect the inherited group after a parent gate, optionally detach and inspect a grandchild's fresh frames.
# The parent always kills/joins this direct child after its result; no actor remains behind a completed test.
# RU: После сигнала родителя проверить группу; при необходимости отделить её и проверить внука.
# Родитель всегда завершает своего ребёнка через killwait после получения результата.
child(done: chan of (int, int), gate: chan of int, grandchild: int)
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    <-gate;
    good := visible(sys, "/private");
    if (Mode == 0 || Mode == 3) {
        good = good && visible(sys, "/shared");
    }
    if (Mode == 2) {
        good = good && sys->open("/parentonly", Sys->OREAD) == nil;
    }
    if (Mode == 1) {
        if (grandchild) {
            good = good && visible(sys, "/childonly");
        } else {
            good = good && sys->pctl(Sys->FORKNS, nil) >= 0;
            good = good && sys->bind("/data", "/childonly", 0) >= 0;
            nested := chan of (int, int);
            next := chan of int;
            spawn child(nested, next, 1);
            next <-= 1;
            (pid, result) := <-nested;
            retired := ctl->killwait(pid);
            good = good && result && retired == Processctl->Retired;
        }
    }
    done <-= (ctl->pid(), good);
    never := chan of int;
    <-never;
}


# EN/RU: Fork the bound instance view, spawn from this same actual module, then check sharing and one-sided detach.
# Return the ordinary typed dependency's value; failure is a precise semantic exception after child retirement.
# RU: Отделить окружение привязанного экземпляра, создать ребёнка из того же модуля и проверить наследование.
# Вернуть значение типизированной зависимости; при ошибке бросить исключение после завершения ребёнка.
value(): int
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    if (sys->pctl(Sys->FORKNS, nil) < 0 || sys->bind("/data", "/private", 0) < 0) {
        raise "cannot prepare bound parent namespace";
    }
    done := chan of (int, int);
    gate := chan of int;
    if (Mode == 3) {
        refused := 0;
        {
            spawn child(done, gate, 0);
        } exception {
            "cannot initialize process namespace" => refused = 1;
        }
        if (!refused) {
            raise "namespace constructor allocation refusal was not observed";
        }
    }
    spawn child(done, gate, 0);
    if ((Mode == 0 || Mode == 3) && sys->bind("/data", "/shared", 0) < 0) {
        raise "cannot extend shared namespace";
    }
    if (Mode == 2 && (sys->pctl(Sys->FORKNS, nil) < 0 || sys->bind("/data", "/parentonly", 0) < 0)) {
        raise "cannot detach parent namespace";
    }
    gate <-= 1;
    (pid, result) := <-done;
    if (ctl->killwait(pid) != Processctl->Retired || !result) {
        raise "bound child lost private namespace";
    }
    if (sys->open("/childonly", Sys->OREAD) != nil) {
        raise "child namespace edits escaped into parent";
    }
    dependency := load Value "/dep.dis";
    if (dependency == nil) {
        return -1;
    }
    return dependency->value();
}
