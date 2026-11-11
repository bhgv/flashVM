# Copyright (c) 2026 bhgv
# EN: Reuse managed file slots while a native close worker accumulates a backlog.
# RU: Повторно использовать слоты файлов при накоплении очереди native close.
implement CloseBacklog;
include "sys.m";
include "processctl.m";

CloseBacklog: module { init: fn(); };
sys: Sys; # Immutable builtin interfaces published before spawn.
control: Processctl; # Exact producer retirement before the host GC checkpoint.
Mixed: con 0; # Driver also tests alternating file and directory jobs.
Files: con 64; # Exceeds the 29 non-stdio descriptor slots.


# EN: Own one file, write an exact private payload and publish PID before exit.
# id selects the path/data; done is a shared copied-identity reply channel.
# RU: Владеть одним файлом, записать точные данные и сообщить PID до выхода.
# id задаёт путь/данные; done передаёт общую копию идентификатора.
producer(id: int, done: chan of int)
{
    fd: ref Sys->FD;
    if (Mixed && id % 2) {
        fd = sys->open("backlog-dir-" + string id, Sys->OREAD);
    } else {
        fd = sys->create("backlog-" + string id, Sys->OWRITE, 8r666);
    }
    if (fd == nil) {
        raise "native close backlog retained a managed descriptor slot";
    }
    if (Mixed && id % 2) {
        (count, entries) := sys->dirread(fd);
        if (count != 1 || len entries != 1 || entries[0].name != "entry") {
            raise "native close backlog changed directory cursor payload";
        }
    } else {
        bytes := array[32] of { * => byte (id + 1) };
        if (sys->write(fd, bytes, len bytes) != len bytes) {
            raise "native close backlog changed producer payload";
        }
    }
    fd = nil;
    done <-= control->pid();
}


# EN: Retire each producer, then wait for observed native job publication through
# the host's genuine werrstr checkpoint. The first native close remains blocked.
# RU: Завершать производителей и ждать наблюдаемой публикации native job через
# настоящий werrstr с наблюдателем хоста. Первое native close остаётся задержано.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    armed := chan[105] of int;
    if (armed == nil) {
        raise "native close observer activation failed";
    }
    done := chan of int;
    for (id := 0; id < Files; id++) {
        spawn producer(id, done);
        pid := <-done;
        result := control->killwait(pid);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "close backlog producer did not retire";
        }
        sys->werrstr("native close publication checkpoint");
    }
    sys->print("PASS managed close backlog files=64 owners=64\n");
}
