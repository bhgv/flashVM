# Copyright (c) 2026 bhgv
# EN: Print a real arena snapshot before, during and after an ordinary byte allocation.
# RU: Печатать real arena snapshot до, во время и после обычного byte allocation.
implement MemoryExample;
include "sys.m";
include "memoryview.m";
include "processctl.m";
MemoryExample: module { init: fn(); };


# EN/RU: Print one captured record and its exact partition; labels borrow immutable text.
# Печатать captured record и exact partition; label заимствует immutable text.
show(sys: Sys, memory: Memoryview, label: string)
{
    (status, stats) := memory->inspect();
    if (status != 1 || stats == nil || stats.arena != stats.header + stats.live + stats.free) {
        raise "memory example unavailable or inconsistent partition";
    }
    sys->print("%s: arena=%bd header=%bd live-span=%bd free-span=%bd blocks=%bd free-blocks=%bd\n",
               label, stats.arena, stats.header, stats.live, stats.free, stats.liveblocks, stats.freeblocks);
    sys->print("  native-overhead/block=%bd live-capacity=%bd free-capacity=%bd allocations=%bd releases=%bd peak=%bd flags=%d\n",
               stats.overhead, stats.livecapacity, stats.freecapacity, stats.allocations, stats.releases, stats.peak, stats.flags);
}


# EN: Keep bytes live through the middle sample, then release. Reclamation timing
# depends on ownership/collector; do not infer requested payload or RSS from spans.
# RU: Сохранить bytes до middle sample, затем отпустить. Время reclamation зависит
# от ownership/collector; spans не объявляются requested payload либо RSS.
worker(done: chan of int)
{
    sys := load Sys Sys->PATH;
    memory := load Memoryview Memoryview->PATH;
    show(sys, memory, "before");
    bytes := array[4096] of byte;
    bytes[0] = byte 37;
    show(sys, memory, "held");
    if (bytes[0] != byte 37) {
        raise "memory example payload changed";
    }
    bytes = nil;
    show(sys, memory, "released");
    ctl := load Processctl Processctl->PATH;
    done <-= ctl->pid();
    hold := chan of int;
    <-hold;
}


# EN/RU: Run real resident sampling, join it, and report success after retirement.
# Снять counters из настоящего resident, завершить его, сообщить успех после retirement.
init()
{
    ctl := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    done := chan of int;
    spawn worker(done);
    if (ctl->killwait(<-done) != ctl->Retired) {
        raise "memory example worker retirement failed";
    }
    sys->print("PASS memory statistics example\n");
}
