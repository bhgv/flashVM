# Copyright (c) 2026 bhgv
# EN: Process-private errors across spawn inheritance, native wait and handled unwind.
# RU: Личные ошибки Prog при наследовании spawn, native-ожидании и обработанном unwind.
implement ResidentErrors;
include "sys.m";
include "processctl.m";

ResidentErrors: module { init: fn(); };
sys: Sys; # Immutable builtin interfaces initialized before spawn.
control: Processctl; # Exact worker retirement after all error checks.
Workers: con 8; # Permanently assigned concurrent OS residents.
Rounds: con 64; # Each iteration has one real native sleep and handled exception.


# EN: Check inherited error, then keep text/roots private across waits and unwind.
# id/initial select this resident's values; channels borrow shared reply owners.
# RU: Проверить наследованную ошибку, затем хранить личные текст/корни при ожидании
# и unwind. id/initial задают значения потока; каналы заимствуют общие ответы.
worker(id: int, initial: string, ready, start, done: chan of int)
{
    if (sys->sprint("%r") != initial) {
        raise "spawn inherited another resident's error text";
    }
    roots := array[16] of string;
    for (at := 0; at < len roots; at++) {
        roots[at] = initial + ":" + string at;
    }
    ready <-= control->pid();
    <-start;
    for (round := 0; round < Rounds; round++) {
        own := "resident-" + string id + "-round-" + string round;
        sys->werrstr(own);
        if (sys->sleep(1) != 0 || sys->sprint("%r") != own) {
            raise "native wait replaced this resident's error text";
        }
        {
            raise own;
        } exception caught {
            "*" => {
                if (caught != own) {
                    raise "resident caught another thread's exception text";
                }
            }
        }
        sys->werrstr(own + ":handled");
        if (sys->sprint("%r") != own + ":handled") {
            raise "handled unwind lost this resident's error environment";
        }
        for (at = 0; at < len roots; at++) {
            if (roots[at] != initial + ":" + string at) {
                raise "resident roots changed across private error unwind";
            }
        }
    }
    done <-= control->pid();
}


# EN: Snapshot each parent's error at spawn, then change only the parent's copy.
# Release all children together and require exact final retirement.
# RU: Снять ошибку родителя при spawn и менять только его копию. Разрешить общий
# старт детей и потребовать точное окончательное завершение.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan of int;
    start := chan of int;
    done := chan of int;
    last := "";
    for (id := 0; id < Workers; id++) {
        inherited := "spawn-error-" + string id;
        sys->werrstr(inherited);
        spawn worker(id, inherited, ready, start, done);
        last = "parent-after-" + string id;
        sys->werrstr(last);
        <-ready;
    }
    if (sys->sprint("%r") != last) {
        raise "child error mutation changed its parent";
    }
    for (id = 0; id < Workers; id++) {
        start <-= 1;
    }
    for (id = 0; id < Workers; id++) {
        pid := <-done;
        result := control->killwait(pid);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "error-state resident did not retire";
        }
    }
    sys->print("PASS resident errors workers=8 inherited=8 unwinds=512\n");
}
