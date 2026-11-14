# Copyright (c) 2026 bhgv
# EN: Retain a real test-injected FP RX fault while another owner allocates continuously.
# RU: Сохранить настоящий test-injected FP RX fault при непрерывных allocations другого owner.
implement JitFpDemo;
include "sys.m";
include "processctl.m";
JitFpDemo: module { init: fn(); };
sys: Sys; # Immutable builtin loaded before spawning native owners.
control: Processctl; # Exact child lifetime controls.


# EN/RU: Return real input plus7 using an emitted function, not a source inline expression.
# Вернуть real input плюс7 отдельной emitted function, не source inline expression.
bad(input: real): real
{
    value := input;
    for (round := 0; round < 7; round++) {
        value += 1.0;
    }
    return value;
}


# EN/RU: Keep independent native code available after the defective function is quarantined.
# Сохранить независимый native code после карантина сбойной функции.
good(input: real): real
{
    value := input;
    for (round := 0; round < 11; round++) {
        value += 1.0;
    }
    return value;
}


# EN/RU: Allocate rooted short-lived arrays continuously until parent terminates this owner.
# Выделять rooted short-lived arrays до завершения этого owner родителем.
allocator(ids: chan of int)
{
    ids <-= control->pid();
    ring := array[8] of array of byte;
    value := 0;
    for (;;) {
        ring[value % 8] = array[64] of byte;
        value = (value + 1) & 65535;
    }
}


# EN: Print fault source identity, preserve an array across the original native error,
# then verify normal catch, subsequent function fallback and independent JIT.
# RU: Вывести identity сбойного source, сохранить array через original native error,
# проверить normal catch, function fallback и независимый JIT.
faulter(done: chan of int)
{
    payload := array[32] of byte;
    payload[0] = byte 91;
    payload[31] = byte 173;
    sys->print("JITSIGNALTARGET pid=%d\n", control->pid());
    caught := 0;
    {
        if (bad(1.0) != 8.0) {
            raise "initial scalar result changed";
        }
    } exception {
        "*" => caught = 1;
    }
    second := 0;
    {
        if (good(3.0) != 14.0) {
            raise "initial independent result changed";
        }
    } exception {
        "*" => second = 1;
    }
    if (payload[0] != byte 91 || payload[31] != byte 173 || bad(2.0) != 9.0 || good(3.0) != 14.0) {
        raise "native signal changed rooted data or scalar results";
    }
    sys->print("JITSIGNALDONE caught=%d second=%d results=9,14 payload=91,173\n", caught, second);
    done <-= control->pid();
}


# EN/RU: Own two native children, join the faulter and explicitly terminate the allocator.
# Владеть двумя native children, дождаться faulter и явно завершить allocator.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    done := chan of int;
    ids := chan of int;
    spawn allocator(ids);
    spin := <-ids;
    spawn faulter(done);
    failed := <-done;
    status := control->killwait(failed);
    if (status != Processctl->Retired && status != Processctl->Denied) {
        raise "native faulter did not retire";
    }
    status = control->killwait(spin);
    if (status != Processctl->Retired && status != Processctl->Denied) {
        raise "native allocator did not retire";
    }
}
