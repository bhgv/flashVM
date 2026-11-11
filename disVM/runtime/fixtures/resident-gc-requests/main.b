# Copyright (c) 2026 bhgv
# EN: Retain private mapped roots across real GC drain and parallel request bursts.
# RU: Сохранять личные mapped-корни при настоящем GC-дренаже и параллельных запросах.
implement GCRequests;
include "sys.m";
include "processctl.m";

GCRequests: module { init: fn(); };
sys: Sys; # Immutable interfaces initialized before thread publication.
control: Processctl; # Parent waits for each permanent resident to retire.
Workers: con 8; # Real rooted native callers.


# EN: A typed mapped child retained only through its owner's local array.
# RU: Типизированный mapped-ребёнок, удерживаемый личным массивом владельца.
Cell: adt {
    value: int; # Private owner/index checksum, initialized before native publication.
};


# EN: Hold the root graph and error text while the native observer requests GC.
# id selects private values; done carries copied PID only after every root check.
# RU: Хранить граф корней и текст ошибки при native-запросах GC наблюдателя.
# id задаёт личные значения; done передаёт PID только после проверки всех корней.
worker(id: int, done: chan of int)
{
    roots := array[32] of ref Cell;
    for (at := 0; at < len roots; at++) {
        roots[at] = ref Cell(id * 1000 + at);
    }
    text := "rooted-gc-owner-" + string id;
    sys->werrstr(text);
    if (sys->sprint("%r") != text) {
        raise "GC request burst replaced this resident's error environment";
    }
    for (at = 0; at < len roots; at++) {
        if (roots[at] == nil || roots[at].value != id * 1000 + at) {
            raise "GC request burst lost a native frame's private root graph";
        }
    }
    done <-= control->pid();
}


# EN: Start eight permanent owners, wait for their real native checkpoints and retire.
# RU: Запустить восемь владельцев, дождаться настоящих native-проверок и завершить.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    done := chan of int;
    for (id := 0; id < Workers; id++) {
        spawn worker(id, done);
    }
    for (id = 0; id < Workers; id++) {
        pid := <-done;
        result := control->killwait(pid);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "GC request resident did not retire";
        }
    }
    sys->print("PASS rooted GC request bursts owners=8 roots=256\n");
}
