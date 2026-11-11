# Copyright (c) 2026 bhgv
# EN: Concurrent cold and repeated module loads with independent MP and retirement.
# RU: Параллельные холодные/повторные загрузки с независимыми MP и завершением.
implement ResidentLoad;
include "sys.m";
include "processctl.m";
include "provider.m";
ResidentLoad: module { init: fn(); };
sys: Sys; # Immutable builtin interface initialized before spawn.
control: Processctl; # Direct-child retirement after final module reference drops.
Workers: con 8; # Simultaneous loading residents, below native descriptor quota.
Rounds: con 32; # Repeated cache lookups and module instance retirement.


# EN: Wait on start, load shared image paths and verify private MP across calls.
# id selects values; ready/start/done remain rooted until this resident exits.
# RU: Ждать start, загружать общие пути и проверять личный MP при вызовах.
# id выбирает значения; ready/start/done укоренены до выхода потока.
worker(id: int, ready, start, done: chan of int)
{
    ready <-= control->pid();
    <-start;
    for (round := 0; round < Rounds; round++) {
        if (round % 8 == 0) {
            missing := load Burst "missing-provider.dis";
            if (missing != nil) {
                raise "missing module load unexpectedly published an instance";
            }
            incompatible := load Burst "bad-provider.dis";
            if (incompatible != nil) {
                raise "incompatible module load unexpectedly published an instance";
            }
        }
        provider := load Burst "provider-" + string (round % 4) + ".dis";
        if (provider == nil) {
            raise "concurrent module load failed";
        }
        value := id * Rounds + round;
        if (provider->run(value) != value * 3 + 7) {
            raise "concurrent loaded module return changed";
        }
        provider = nil;
    }
    done <-= control->pid();
}


# EN: Start eight permanent residents together and wait for complete retirement.
# RU: Запустить восемь закреплённых потоков вместе и дождаться их завершения.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan of int;
    start := chan of int;
    done := chan of int;
    for (id := 0; id < Workers; id++) {
        spawn worker(id, ready, start, done);
    }
    for (id = 0; id < Workers; id++) {
        <-ready;
    }
    for (id = 0; id < Workers; id++) {
        start <-= 1;
    }
    for (id = 0; id < Workers; id++) {
        pid := <-done;
        result := control->killwait(pid);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "module loading resident did not retire";
        }
    }
    sys->print("PASS resident module loads workers=8 loads=256 images=4\n");
}
