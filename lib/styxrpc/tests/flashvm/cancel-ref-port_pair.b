# Copyright (c) 2026 bhgv
# EN: Cancel active native .varref waits while the issuing FlashVM still serves its tree.
# RU: Отменить нативные ожидания .varref, пока выдавшая сторона FlashVM обслуживает дерево.
implement Client;
include "sys.m";
include "processctl.m";
include "styxmodref.m";

Client: module { init: fn(); };
sys: Sys; # Namespace, paired control and monotonic clock.
control: Processctl; # Cancel only an admitted wait of a direct child.
refs: Styxmodref; # Guest view of the shared C SDK port.
Cell: import refs;
Export: con "mounted/ref-port.dis/.var/counter"; # Same name across a real restart.
Budget: con 2000; # Milliseconds, below the Styx request deadline.
Poll: con 5; # Milliseconds between readiness probes.

include "named_movie_upload.b";


# EN: Wait until the movie publishes this path with a new Qid.
# RU: Дождаться публикации пути фильмом с новым Qid.
publication(previous: big): big
{
    began := sys->millisec();
    while (sys->millisec() - began < Budget) {
        (ok, meta) := sys->stat("/peer/" + Export);
        if (ok == 0 && meta.qid.path != previous && meta.length == big 4) {
            return meta.qid.path;
        }
        sys->sleep(Poll);
    }
    raise "missing FlashVM Int32 publication";
}


# EN: Wait for the actual original relay reply, then verify separate mounted traffic.
# RU: Дождаться исходного ответа на барьере, затем проверить соседний обмен.
wait_gate(stage: int)
{
    bytes := array[1] of byte;
    began := sys->millisec();
    while (sys->millisec() - began < Budget) {
        fd := sys->open("/control-gate", Sys->OREAD);
        if (fd == nil || sys->read(fd, bytes, 1) != 1) {
            raise "cannot inspect reference cancellation gate";
        }
        if (int bytes[0] == '1' + stage) {
            (ok, meta) := sys->stat("/peer/mounted/ref-port.dis");
            if (ok != 0) {
                raise "pending reference stalled a separate fid";
            }
            return;
        }
        sys->sleep(Poll);
    }
    raise sys->sprint("missing reference cancellation stage %d (observed byte %d)", stage, int bytes[0]);
}


# EN: Operate on the borrowed port in one direct child. Report failure or full completion.
# RU: Обратиться к заимствованному порту в прямом потомке. Сообщить отказ или успех.
transfer(stage: int, alias: ref Cell, started, done: chan of int)
{
    started <-= control->pid();
    {
        if (stage < 2) {
            cell := refs->capture(Export, 1);
            cell.close();
        } else if (stage < 4) {
            value := alias.read();
        } else {
            alias.write(42);
        }
    } exception e {
    "*" =>
        done <-= 1;
        return;
    }
    done <-= 0;
}


# EN: Compare the same opaque named frame before and after cancellation.
# RU: Сравнить непрозрачный именованный кадр до и после отмены.
same(first, next: array of byte): int
{
    if (len first != len next) {
        return 0;
    }
    for (i := 0; i < len first; ++i) {
        if (first[i] != next[i]) {
            return 0;
        }
    }
    return 1;
}


# EN: Probe the old binding once after the publication has been replaced.
# RU: Один раз проверить старую привязку после замены публикации.
readable(alias: ref Cell): int
{
    {
        value := alias.read();
    } exception e {
    "*" =>
        return 0;
    }
    return 1;
}


# EN: Restart the actual FlashVM movie, preserving the connection and its named scope.
# RU: Перезапустить настоящий фильм FlashVM с сохранением подключения и его области.
restart()
{
    fd := sys->open("/peer/.state", Sys->OWRITE);
    bytes := array of byte "restart";
    if (fd == nil || sys->write(fd, bytes, len bytes) != len bytes) {
        raise "cannot replace the FlashVM publication";
    }
}


# EN: Drive capture, lazy resolution and value cancellation, then reuse the same port.
# RU: Отменить захват, отложенное открытие и обмен значением, затем использовать тот же порт.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    refs = load Styxmodref Styxmodref->PATH;
    if (control == nil || refs == nil) {
        raise "cannot load reference or process control support";
    }
    refs->init();
    start("cancel-ref-port.swf");
    first := publication(big 0);
    seed := refs->capture(Export, 1);
    frame := seed.payload();
    seed.close();
    alias := refs->adopt(frame);
    stale := refs->adopt(frame);
    arm := sys->create("/arm-ref-port", Sys->OWRITE, 8r666);
    if (arm == nil) {
        raise "cannot arm the reference gate";
    }
    arm = nil;
    started := chan[1] of int;
    done := chan[1] of int;
    for (stage := 0; stage < 5; ++stage) {
        spawn transfer(stage, alias, started, done);
        pid := <-started;
        wait_gate(stage);
        began := sys->millisec();
        if (control->cancelwait(pid) != Processctl->Cancelled || <-done != 1 ||
            sys->millisec() - began >= Budget) {
            raise "native reference wait was not cancelled at stage " + string stage;
        }
    }
    if (!same(frame, alias.payload()) || alias.read() != 42) {
        raise "the exact port did not recover the original reference and assignment";
    }
    alias.write(73);
    if (alias.read() != 73) {
        raise "recovery assignment did not reach FlashVM";
    }
    restart();
    publication(first);
    if (readable(stale) || readable(alias)) {
        raise "an old reference accepted the replacement's Qid";
    }
    fresh := refs->capture(Export, 0);
    if (fresh.read() != 17) {
        raise "the new publication did not have its own state";
    }
    fresh.close();
    stale.close();
    alias.close();
    sys->print("PASS native cancel-ref-port RPC paired VMs\n");
}
