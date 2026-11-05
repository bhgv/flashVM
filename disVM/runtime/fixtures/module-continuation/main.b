implement Main;
include "sys.m";
include "provider.m";

Main: module { init: fn(); };
sys: Sys;

peer(provider: Provider)
{
    if (provider->touch() != 314159)
        raise "peer call failed";
    provider = nil;
    sys->print("PEER released provider\n");
}

init()
{
    sys = load Sys Sys->PATH;
    provider := load Provider "provider.dis";
    if (provider == nil)
        raise "cannot load provider";
    spawn peer(provider);
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
    provider = nil;
}
