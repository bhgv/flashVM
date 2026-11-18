# Copyright (c) 2026 bhgv
# EN: Ordinary local startup has no accepted launch context, including repeated result replacement in a child.
# RU: Обычный local startup не имеет accepted launch context, включая repeated result replacement в child.
implement Check;
include "sys.m";
include "launchcontext.m";
Check: module
{
    init: fn(); # Standard host bootstrap signature.
};


# EN/RU: Require nil results without inventing labels/argument data; signal completion through an ordinary channel.
# Требовать nil без выдуманных labels/arguments; сообщить completion обычным channel.
child(launch: Launchcontext, done: chan of int)
{
    for (i := 0; i < 16; i++) {
        name := "previous";
        bytes := array[3] of byte;
        name = launch->name();
        bytes = launch->arguments();
        if (name != nil || bytes != nil) {
            raise "ordinary startup invented launch context";
        }
    }
    done <-= 1;
}


# EN/RU: Execute actual builtin on ordinary initial owner and child without a native handoff.
# Выполнить настоящий builtin на ordinary initial owner и child без native handoff.
init()
{
    sys := load Sys Sys->PATH;
    launch := load Launchcontext Launchcontext->PATH;
    done := chan of int;
    spawn child(launch, done);
    if (launch == nil || launch->name() != nil || launch->arguments() != nil || <-done != 1) {
        raise "absent startup context failed";
    }
    sys->print("PASS absent launch context\n");
}
