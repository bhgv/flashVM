# Copyright (c) 2026 bhgv
# EN: Original network owner callback with visible admission and partial effects.
# RU: Исходный network owner callback с наблюдаемым admission и частичными effects.
implement Proxyhandler;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";


# EN/RU: Bind original capture to this serving permanent Prog identity.
# Привязать исходный capture к identity обслуживающего постоянного Prog.
start(capture: ref Proxyhost->Cell)
{
    control := load Processctl Processctl->PATH;
    capture.value = control->pid();
}


# EN: Mutate original alias refs only on the captured owner. -999 publishes entry
# and blocks until connection cleanup kills this owner; -1 raises after one effect.
# RU: Менять исходные alias refs лишь на captured owner. -999 сообщает entry и
# блокируется до kill при cleanup соединения; -1 бросает после одного effect.
invoke(capture, left, right: ref Proxyhost->Cell, value: int): int
{
    control := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    if (capture.value != control->pid() || left == nil || left != right) {
        raise "network callback lost original owner or aliases";
    }
    if (value == -999) {
        left.value = 777;
        sys->fprint(sys->fildes(2), "NETWORK callback admitted and blocked\n");
        sys->sleep(60000);
        raise "abandoned network callback resumed";
    }
    left.value += value;
    if (value == -1) {
        raise "network partial callback effect";
    }
    right.value += value;
    return left.value;
}


# EN: Return a real new variable/capture on this owner; -888 returns original
# left alias, -1 raises after mutation, -999 exposes actual callback admission.
# RU: Вернуть real new variable/capture на owner; -888 возвращает original left,
# -1 бросает после mutation, -999 наблюдаемо входит в actual callback.
result(capture, left, right: ref Proxyhost->Cell, value, kind: int): ref Proxyhost->Cell
{
    control := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    if (capture.value != control->pid() || left == nil || left != right) {
        raise "result callback lost original capture/ref owner";
    }
    if (value == -999) {
        left.value = 777;
        sys->fprint(sys->fildes(2), "NETWORK result callback admitted and blocked\n");
        sys->sleep(60000);
        raise "abandoned result callback resumed";
    }
    if (value == -1) {
        left.value++;
        raise "result callback partial effect";
    }
    if (value == -889) {
        left.value++;
        return left;
    }
    if (value == -888) {
        return left;
    }
    if (value == -2) {
        return nil;
    }
    if (kind == Proxyhost->Function) {
        return ref Proxyhost->Cell(capture.value);
    }
    return ref Proxyhost->Cell(value);
}


# EN: Execute on original captured owner, accessing actual foreign variables through
# typed variable-only refs. Each operation uses the original absolute deadline.
# RU: Исполнить на original captured owner, обращаться к actual foreign variables
# через typed variable-only refs. Все операции используют original absolute deadline.
routed(capture: ref Proxyhost->Cell, left, right: ref Proxyref->Ref,
       refs, value, deadline: int): int
{
    control := load Processctl Processctl->PATH;
    access := load Proxyref Proxyref->PATH;
    sys := load Sys Sys->PATH;
    access->init();
    sys->fprint(sys->fildes(2), "ROUTED callback entered owner=%d\n", control->pid());
    if (capture.value != control->pid() || refs != 2 || left == nil || right == nil) {
        raise "routed callback lost original capture or typed refs";
    }
    (status, unused) := access->call(left, Proxyref->Add, value, deadline);
    if (status != 0) {
        raise sys->sprint("first routed variable access failed: status=%d remaining=%d", status, deadline - sys->millisec());
    }
    (status, unused) = access->call(right, Proxyref->Add, value, deadline);
    if (status != 0) {
        raise sys->sprint("second routed variable access failed: status=%d remaining=%d", status, deadline - sys->millisec());
    }
    return unused;
}


# EN: Return a live argument after original-owner effects. Sentinels exercise nil,
# an unregistered wrapper, and exception after one actual foreign Add.
# RU: Вернуть live argument после original-owner effects. Sentinels проверяют nil,
# unregistered wrapper и exception после одного actual foreign Add.
routedresult(capture: ref Proxyhost->Cell, left, right: ref Proxyref->Ref,
             refs, value, deadline: int): ref Proxyref->Ref
{
    control := load Processctl Processctl->PATH;
    if (capture.value != control->pid() || refs != 2 || left == nil || right == nil) {
        raise "routed result lost original owner or live arguments";
    }
    if (value == -2) {
        return nil;
    }
    if (value == -3) {
        return ref Proxyref->Ref(left.requests, left.state, left.lease);
    }
    if (value == -1) {
        access := load Proxyref Proxyref->PATH;
        access->init();
        access->call(left, Proxyref->Add, 1, deadline);
        raise "routed result partial effect";
    }
    if (value != -888 && value != -889) {
        routed(capture, left, right, refs, value, deadline);
    }
    if (value == -889 || value == 1) {
        return right;
    }
    return left;
}
