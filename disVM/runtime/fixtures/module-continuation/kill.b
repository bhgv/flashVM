implement Kill;
include "sys.m";
include "processctl.m";
include "provider.m";

Kill: module { init: fn(); };

victim(inbox: chan of Provider, ids: chan of int)
{
    control := load Processctl Processctl->PATH;
    provider := <-inbox;
    ids <-= control->pid();
    provider->wait();
    raise "terminated guest resumed its caller";
}

notify(sys: Sys)
{
    sys->print("PEER released provider\n");
}

init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    provider := load Provider "provider.dis";
    if (provider == nil)
        raise "cannot load provider";
    inbox := chan[1] of Provider;
    ids := chan[1] of int;
    inbox <-= provider;
    spawn victim(inbox, ids);
    pid := <-ids;
    if (provider->touch() != 314159)
        raise "peer call failed";
    provider = nil;
    # Notify from a separate guest after the parent parks in killwait.
    spawn notify(sys);
    if (control->killwait(pid) != Processctl->Retired ||
        control->killwait(pid) != Processctl->Denied)
        raise "provider guest did not retire exactly once";
    # Load the same image again after the child's frames have retired.
    provider = load Provider "provider.dis";
    if (provider == nil || provider->touch() != 314159)
        raise "provider reload failed after retirement";
    sys->print("PASS provider termination and reload\n");
}
