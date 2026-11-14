# Copyright (c) 2026 bhgv
# EN: Observe one coherent native arena partition during real guest allocation churn.
# RU: Наблюдать coherent native arena partition при actual guest allocation churn.
implement MemoryStatistics;
include "sys.m";
include "processctl.m";
include "memoryview.m";
MemoryStatistics: module { init: fn(); };
Refusal: con 0; # Driver compiles explicit native-unavailable output acceptance.
ctl: Processctl; # Immutable matched hosted interface.
sys: Sys; # Immutable scalar output services.


# EN/RU: Shared 96-byte result with untouched adjacent canaries; no observer writes.
# Shared 96-byte result с неизменными соседними canaries; observer ничего не пишет.
Reply: adt {
    before: int; # Leading canary.
    status: int; # One snapshot success/refusal.
    arena: big; # Reserved arena span.
    header: big; # Arena header outside blocks.
    live: big; # Allocated block spans.
    free: big; # Free block spans.
    blocks: big; # Allocated blocks.
    holes: big; # Free blocks.
    overhead: big; # Native per-block overhead.
    allocations: big; # Successful allocation history.
    releases: big; # Successful free history.
    peak: big; # Peak allocated span.
    flags: int; # Saturation flags.
    after: int; # Trailing canary.
};


# EN: Allocate live byte rings while parent samples counters. Ordinary channel
# completion proves every round ran; parent joins each parked direct child.
# RU: Выделять live byte rings во время counters sampling parent. Ordinary channel
# completion доказывает все rounds; parent завершает parked direct child через join.
churn(ready, go, done: chan of int)
{
    ready <-= ctl->pid();
    <-go;
    ring := array[16] of array of byte;
    for (round := 0; round < 300; round++) {
        value := array[64 + (round % 48)*16] of byte;
        value[0] = byte (round % 251);
        ring[round % 16] = value;
        if (ring[round % 16][0] != byte (round % 251)) {
            raise "memory statistics ordinary payload corrupted";
        }
    }
    done <-= ctl->pid();
    hold := chan of int;
    <-hold;
}


# EN: Continuously access shared result/canaries through ordinary managed reads.
# Per-field reads span instants; observer must not demand multi-load consistency.
# RU: Непрерывно читать shared result/canaries обычными managed reads. Отдельные
# fields читаются в разные instants; observer не требует multi-load consistency.
watch(reply: ref Reply, stop: array of int, ready, done: chan of int)
{
    ready <-= ctl->pid();
    for (;;) {
        if (reply.before != 31 || reply.after != 79 || reply.live < big 0 || reply.free < big 0) {
            raise "memory statistics shared scalar result overran canary";
        }
        if (stop[0]) {
            break;
        }
    }
    done <-= ctl->pid();
    hold := chan of int;
    <-hold;
}


# EN: Validate one completed writer's own result; no other writer changes it.
# Native refusal must zero every field. Saturated history cannot imply exact deltas.
# RU: Проверить completed result своего writer; других writers нет. Native refusal
# обязан обнулить все fields. Saturated history не даёт exact deltas.
valid(r: ref Reply): int
{
    if (Refusal) {
        return r.status == -1 && r.arena == big 0 && r.header == big 0 && r.live == big 0 && r.free == big 0 &&
               r.blocks == big 0 && r.holes == big 0 && r.overhead == big 0 && r.allocations == big 0 &&
               r.releases == big 0 && r.peak == big 0 && !r.flags;
    }
    return r.status == 1 && r.arena > big 0 && r.header > big 0 && r.overhead > big 0 &&
           r.arena == r.header + r.live + r.free && r.blocks >= big 0 && r.holes >= big 0 &&
           r.live >= r.blocks*r.overhead && r.free >= r.holes*r.overhead && r.peak >= r.live &&
           r.peak <= r.arena - r.header && !r.flags && r.allocations >= r.releases &&
           r.allocations - r.releases == r.blocks;
}


# EN: Run four allocating guests plus one shared-result observer. Detect omissions
# only after normal child joins; require independent library copy and traffic progress.
# RU: Исполнить four allocating guests и shared-result observer. Обнаруживать
# omissions только после normal child joins; проверить library copy и traffic progress.
init()
{
    ctl = load Processctl Processctl->PATH;
    sys = load Sys Sys->PATH;
    memory := load Memoryview Memoryview->PATH;
    ready := chan of int;
    go := chan of int;
    done := chan of int;
    ids := array[4] of int;
    for (index := 0; index < 4; index++) {
        spawn churn(ready, go, done);
        ids[index] = <-ready;
    }
    reply := ref Reply(31, 0, big 0, big 0, big 0, big 0, big 0, big 0, big 0, big 0, big 0, big 0, 0, 79);
    stop := array[1] of {* => 0};
    spawn watch(reply, stop, ready, done);
    watcher := <-ready;
    for (index = 0; index < 4; index++) {
        go <-= 1;
    }
    good := 1;
    previous := big 0;
    for (sample := 0; sample < 200; sample++) {
        (reply.status, reply.arena, reply.header, reply.live, reply.free, reply.blocks, reply.holes,
         reply.overhead, reply.allocations, reply.releases, reply.peak, reply.flags) = ctl->memorysnapshot();
        if (!valid(reply) || reply.allocations < previous) {
            good = 0;
        }
        previous = reply.allocations;
    }
    (viewstatus, view) := memory->inspect();
    if (Refusal) {
        if (viewstatus != -1 || view != nil) {
            raise "memory statistics refusal library returned partial data";
        }
    } else if (good && (viewstatus != 1 || view == nil || view.arena != view.header + view.live + view.free ||
               view.livecapacity != view.live - view.liveblocks*view.overhead ||
               view.freecapacity != view.free - view.freeblocks*view.overhead || view.allocations < previous)) {
        raise "memory statistics library mixed accounting instants";
    }
    for (index = 0; index < 4; index++) {
        pid := <-done;
        found := 0;
        for (other := 0; other < 4; other++) {
            if (ids[other] == pid) {
                found = 1;
                ids[other] = 0;
            }
        }
        if (!found || ctl->killwait(pid) != ctl->Retired) {
            raise "memory statistics allocation guest join failed";
        }
    }
    stop[0] = 1;
    if (<-done != watcher || ctl->killwait(watcher) != ctl->Retired || reply.before != 31 || reply.after != 79) {
        raise "memory statistics observer join or canary failed";
    }
    if (!good) {
        sys->print("DETECTED memory statistics inconsistent copied accounting\n");
        return;
    }
    if (Refusal) {
        sys->print("PASS memory statistics refusal: zero output nil library ordinary work joined\n");
    } else {
        sys->print("PASS memory statistics: coherent partition traffic shared result library ordinary work joined\n");
    }
}
