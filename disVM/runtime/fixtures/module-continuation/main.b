implement Main;
include "sys.m";
include "provider.m";

Main: module { init: fn(); };
sys: Sys;
mailbox: (Provider, array of int);
returnbox: (Provider, array of int);
outbox: (Provider, array of int);
module_code: byte;

peer(inbox: chan of (Provider, array of int), reply: chan of array of int)
{
    (provider, payload) := <-inbox;
    if (payload[0] != 161803)
        raise "buffered handoff lost its payload";
    if (provider->touch() != 314159)
        raise "peer call failed";
    provider = nil;
    payload[0] = 271828;
    reply <-= payload;
    payload = nil;
    sys->print("PEER released provider\n");
}

rendezvous(outbound, inbound: chan of (Provider, array of int), ready: chan of int)
{
    ready <-= 1;
    mailbox = <-outbound;
    (provider, payload) := mailbox;
    mailbox = (nil, nil);
    payload[0] = 314159;
    ready <-= 2;
    returnbox = (provider, payload);
    inbound <-= returnbox;
    returnbox = (nil, nil);
}

# Each guest waits on its own channel. The completion channel reports identity.
channel_guest(input, completed: chan of int, expected: int)
{
    if (<-input != expected)
        raise "channel delivered to the wrong guest";
    completed <-= expected;
}

check_channels()
{
    inputs := array[40] of chan of int;
    completed := chan of int;
    for (i := 0; i < len inputs; i++)
    {
        inputs[i] = chan of int;
        spawn channel_guest(inputs[i], completed, i);
    }
    for (i = 0; i < len inputs; i++)
        inputs[i] <-= i;
    seen := array[40] of {* => 0};
    for (i = 0; i < len inputs; i++)
    {
        value := <-completed;
        if (value < 0 || value >= len seen || seen[value] != 0)
            raise "channel completion was lost or duplicated";
        seen[value] = 1;
    }
}

init()
{
    sys = load Sys Sys->PATH;
    provider := load Provider "provider.dis";
    if (provider == nil)
        raise "cannot load provider";
    inbox := chan[1] of (Provider, array of int);
    payload := array[] of {161803};
    inbox <-= (provider, payload);
    payload = nil;
    reply := chan[1] of array of int;
    spawn peer(inbox, reply);
    caller := array[] of {161803};
    {
        if (provider->wait() != 585987 || caller[0] != 161803)
            raise "caller state lost after provider return";
        sys->print("PASS provider return\n");
    } exception failure {
        "provider exception after native wait" =>
            if (caller[0] != 161803 || provider->touch() != 314159)
                raise "caller state lost after provider unwind";
            sys->print("PASS provider unwind\n");
        * => raise;
    }
    payload = <-reply;
    if (payload[0] != 271828)
        raise "buffered reply lost its payload";
    # The selected guards both borrow typed records from this frame.
    message := (provider, payload);
    alt {
        inbox <-= message => {}
        * => raise "ready ALT send was skipped";
    }
    received: (Provider, array of int);
    alt {
        received = <-inbox => {}
    }
    (result, values) := received;
    if (result != provider || values[0] != 271828)
        raise "ALT changed the transferred owners";
    outbound := chan of (Provider, array of int);
    inbound := chan of (Provider, array of int);
    ready := chan[1] of int;
    spawn rendezvous(outbound, inbound, ready);
    if (<-ready != 1)
        raise "rendezvous receiver did not start";
    outbox = (provider, payload);
    outbound <-= outbox;
    outbox = (nil, nil);
    if (<-ready != 2)
        raise "rendezvous sender did not start";
    (back, changed) := <-inbound;
    if (back != provider || changed[0] != 314159)
        raise "rendezvous lost the transferred owners";
    raw := chan[1] of byte;
    code := byte changed[0];
    module_code = code;
    raw <-= code;
    code = <-raw;
    module_code = code;
    if (module_code != byte 314159)
        raise "raw buffered channel lost its byte";
    provider = nil;
    check_channels();
    sys->print("PASS forty independent channels\n");
}
