# Copyright (c) 2026 bhgv
# EN: Independent loop, wait and managed fault keep real continuations for host debugger.
# RU: Independent loop, wait и managed fault сохраняют continuations для host debugger.
implement RetainedDebug;
include "sys.m";
include "processctl.m";
RetainedDebug: module { init: fn(); };


# EN/RU: A self-cycle and aliases must be deduplicated by on-demand root census.
# Self-cycle и aliases должны deduplicate в root census по запросу.
Node: adt {
    value: int; # Stable payload used after the failing operation.
    next: cyclic ref Node; # Self-reference, never recursive host traversal.
    data: array of byte; # Shared alias of the same retained payload.
};


# EN/RU: Run forever through real instruction boundaries; no diagnostic guest channel.
# Исполнять цикл через actual instruction boundaries; без diagnostic guest channel.
spinner(ready: chan of int)
{
    ctl := load Processctl Processctl->PATH;
    ready <-= ctl->pid();
    value := 0;
    ring := array[8] of array of byte;
    for (;;) {
        ring[value % 8] = array[64] of byte;
        value = (value + 1) & 65535;
    }
}


# EN/RU: Keep original locals/object alive at a failing instruction before unwind.
# Сохранить original locals/object на failing instruction до unwind.
crash(value: int): int
{
    canary := big "1234605616436508552";
    payload := array[32] of byte;
    payload[0] = byte 91;
    payload[31] = byte 173;
    node := ref Node(12345, nil, payload);
    node.next = node;
    alias := node;
    zero := value - value;
    result := value / zero;
    return result + int canary + int payload[0] + alias.value;
}


# EN/RU: Announce fault PID then enter a nested managed frame; parent remains independent.
# Сообщить fault PID, войти во вложенный managed frame; parent независим.
fault(ready: chan of int)
{
    ctl := load Processctl Processctl->PATH;
    ready <-= ctl->pid();
    if (crash(17) == 0) {
        raise "unexpected fault result";
    }
}


# EN/RU: Native sleep publishes roots; debugger must inspect without cancelling sleep.
# Native sleep публикует roots; debugger должен inspect без отмены sleep.
sleeper(ready: chan of int)
{
    ctl := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    ready <-= ctl->pid();
    sys->sleep(120000);
    raise "sleep unexpectedly completed";
}


# EN/RU: First-chance stop must precede a normal catch, then continue through handler.
# First-chance stop должен предшествовать обычному catch, затем продолжить handler.
caught(ready: chan of int)
{
    ctl := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    ready <-= ctl->pid();
    {
        raise "debug caught λ marker";
    } exception {
        "debug caught λ marker" => sys->print("DEBUGCAUGHT normal handler resumed\n");
    }
    hold := chan of int;
    <-hold;
}


# EN/RU: A managed throw from a native builtin must keep its caller/frame roots too.
# Managed throw из native builtin тоже сохраняет caller/frame roots.
builtin(ready: chan of int)
{
    ctl := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    held := array[16] of byte;
    held[0] = byte 201;
    ready <-= ctl->pid();
    sys->chdir(".");
    if (held[0] != byte 201) {
        raise "native caller root changed";
    }
}


# EN/RU: Publish identities via ordinary output, then stay channel-blocked for debug attach.
# Сообщить identities обычным output и ждать channel-blocked debug attach.
init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    ready := chan of int;
    spawn spinner(ready);
    loop := <-ready;
    spawn fault(ready);
    failed := <-ready;
    spawn sleeper(ready);
    native := <-ready;
    spawn caught(ready);
    handled := <-ready;
    spawn builtin(ready);
    nativefault := <-ready;
    sys->print("DEBUGTARGETS main=%d loop=%d fault=%d native=%d caught=%d builtin=%d\n", ctl->pid(), loop, failed, native, handled, nativefault);
    hold := chan of int;
    <-hold;
}
