# Copyright (c) 2026 bhgv
# EN: Real console bootstrap uses independent launch-data copies and child inheritance without display context.
# RU: Настоящий console bootstrap проверяет независимые launch-data copies и child inheritance без дисплея.
implement Check;
include "sys.m";
include "launchcontext.m";
include "processctl.m";
include "vfsexport.m";
Check: module
{
    init: fn(); # Existing zero-argument bootstrap ABI, no frame injection.
};


# EN/RU: Verify one independently mutable payload and repeat label publication on this exact owner.
# Проверить независимый mutable payload и повторную выдачу label на своём owner.
verify(launch: Launchcontext)
{
    name := "previous-result";
    bytes := array[3] of byte;
    for (repeat := 0; repeat < 16; repeat++) {
        name = launch->name();
        bytes = launch->arguments();
        if (name != "console/ordinary-module" || len bytes != ARGUMENT_BYTES) {
            raise "launch snapshot missing or aliased";
        }
        step := 1;
        if (len bytes > 257) {
            step = 4093;
        }
        for (i := 0; i < len bytes; i += step) {
            if (int bytes[i] != (i & 255)) {
                raise "launch payload changed across copies or owners";
            }
            bytes[i] = byte 42;
        }
        if (len bytes > 257 && int bytes[len bytes - 1] != 255) {
            raise "launch payload tail changed";
        }
    }
}


# EN/RU: Child owns inherited native pin and independent managed copies; parent joins its permanent owner by killwait.
# Child владеет inherited native pin и независимыми managed copies; parent завершает owner через killwait.
child(launch: Launchcontext, ready: chan of int)
{
    verify(launch);
    ctl := load Processctl Processctl->PATH;
    ready <-= ctl->pid();
    never := chan of int;
    <-never;
}


# EN/RU: Exercise binary/empty/maximal data, concurrent child readers, nil FlashVM and ordinary stdin/stdout/stderr.
# Проверить binary/empty/maximal data, concurrent child readers, nil FlashVM и обычные stdin/stdout/stderr.
init()
{
    sys := load Sys Sys->PATH;
    launch := load Launchcontext Launchcontext->PATH;
    ctl := load Processctl Processctl->PATH;
    exports := load Vfsexport Vfsexport->PATH;
    if (launch == nil || exports->terminal() != nil) {
        raise "console-only launch invented display context";
    }
    verify(launch);
    ready := chan of int;
    for (i := 0; i < 4; i++) {
        spawn child(launch, ready);
    }
    verify(launch);
    for (j := 0; j < 4; j++) {
        pid := <-ready;
        if (pid <= 0 || ctl->killwait(pid) != Processctl->Retired) {
            raise "launch context child retirement failed";
        }
    }
    input := array[32] of byte;
    n := sys->read(sys->fildes(0), input, len input);
    if (n != 14 || string input[:n] != "console-input\n") {
        raise "console launch lost stdin";
    }
    out := array of byte "PASS launch context console stdout\n";
    err := array of byte "PASS launch context console stderr\n";
    if (sys->write(sys->fildes(1), out, len out) != len out ||
        sys->write(sys->fildes(2), err, len err) != len err) {
        raise "console launch lost stdout or stderr";
    }
    sys->print("PASS immutable launch context and child copies\n");
}
