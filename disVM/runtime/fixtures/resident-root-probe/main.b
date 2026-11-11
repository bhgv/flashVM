# Copyright (c) 2026 bhgv
# EN: Retire neighboring Prog roots while native observers inspect frozen owners.
# RU: Снимать корни соседних Prog при проверке остановленных владельцев наблюдателями.
implement RootProbe;
include "sys.m";
include "processctl.m";
RootProbe: module { init: fn(); };
sys: Sys; # Immutable native interface loaded before publication.
control: Processctl; # Parent waits for complete retirement between bursts.
Workers: con 8; # Concurrent permanent owners in each burst.
Bursts: con 4; # Repeat registry publication and actual storage retirement.


# EN: Private mapped root retained across native sleep and its real observer.
# RU: Личный mapped-корень, живущий через native sleep и настоящего наблюдателя.
Cell: adt {
    value: int; # Owner checksum set before the native root publication.
};


# EN: Keep id's private graph alive across sleep; done returns checksum/PID.
# RU: Сохранить личный граф id при sleep; done возвращает checksum/PID.
worker(id: int, done: chan of (int, int))
{
    cells := array[16] of ref Cell;
    for (at := 0; at < len cells; at++) {
        cells[at] = ref Cell(id * 16 + at);
    }
    sys->sleep(1);
    for (at = 0; at < len cells; at++) {
        if (cells[at] == nil || cells[at].value != id * 16 + at) {
            raise "native observer lost a frozen owner's private graph";
        }
    }
    done <-= (id, control->pid());
}


# EN: Publish and retire four bursts; retain no dead child between bursts.
# RU: Опубликовать и завершить четыре серии; не удерживать мёртвых детей между ними.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    done := chan of (int, int);
    for (burst := 0; burst < Bursts; burst++) {
        for (id := 0; id < Workers; id++) {
            spawn worker(burst * Workers + id, done);
        }
        seen := array[Workers] of {* => 0};
        for (id = 0; id < Workers; id++) {
            (owner, pid) := <-done;
            owner -= burst * Workers;
            if (owner < 0 || owner >= Workers || seen[owner]) {
                raise "native observer changed resident identity";
            }
            seen[owner] = 1;
            result := control->killwait(pid);
            if (result != Processctl->Retired && result != Processctl->Denied) {
                raise "native observer resident did not retire";
            }
        }
    }
    sys->print("PASS resident root probe bursts=4 owners=32 roots=512\n");
}
