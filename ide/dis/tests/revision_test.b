implement RevisionTest;
include "sys.m";
    sys: Sys;
include "idecore.m";
    core: Idecore;

# A reduced-maximum fixture executes the production guards without billions of requests.
RevisionTest: module
{
    init: fn();
};

# Assertions are counted only in this test bootstrap, never in the production service.
checks := 0;

# Exit the isolated emulator after printing a terminal result.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if(control == nil)
        raise "fail: missing halt control";
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: halt returned";
}

# Fail visibly before terminating the bootstrap's own emulator.
require(ok: int, message: string)
{
    if(!ok){
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}

# Execute one command against the current fixture owner using a fresh reply rendezvous.
exchange(commands: chan of core->Request, operation, expected: int, text: string): core->Reply
{
    response := chan of core->Reply;
    commands <-= core->Request(1, operation, expected, 0, 1, text, response);
    return <-response;
}

# Verify rejected writes/close/reopen, allowed no-ops/save/shutdown and unchanged max state.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil && core->MaxRevision == 4, "reduced maximum fixture selected");
    commands := core->start();
    result := exchange(commands, core->Open, 0, "a");
    require(result.status == core->Ok && result.revision == 1, "open");
    result = exchange(commands, core->Replace, 1, "b");
    require(result.status == core->Ok && result.revision == 2, "edit");
    result = exchange(commands, core->Close, 2, "");
    require(result.status == core->Ok && result.revision == 3, "close");
    result = exchange(commands, core->Open, 3, "c");
    require(result.status == core->Ok && result.revision == 4, "reach maximum");
    result = exchange(commands, core->Replace, 4, "d");
    require(result.status == core->RevisionLimit && result.revision == 4 && result.text == "c",
            "changing edit cannot wrap");
    result = exchange(commands, core->Replace, 4, "c");
    require(result.status == core->Ok && result.revision == 4, "no-op allowed at maximum");
    result = exchange(commands, core->MarkSaved, 4, "");
    require(result.status == core->Ok && result.saved == 4, "save allowed at maximum");
    result = exchange(commands, core->Close, 4, "");
    require(result.status == core->RevisionLimit && result.opened && result.text == "c",
            "close cannot wrap or discard text");
    # Read-only navigation/search remain useful even when revision cannot advance.
    query := chan of core->Reply;
    commands <-= core->Request(0, core->Locate, 4, 1, 0, "", query);
    location := <-query;
    require(location.status == core->Ok && location.offset == 1 && location.column == 1 &&
            location.line == 1 && location.revision == 4, "locate at maximum");
    commands <-= core->Request(0, core->Offset, 4, 1, 1, "", query);
    location = <-query;
    require(location.status == core->Ok && location.offset == 1, "offset at maximum");
    commands <-= core->Request(0, core->Find, 4, 0, 1, "c", query);
    location = <-query;
    require(location.status == core->Ok && location.offset == 0 && location.saved == 4,
            "find at maximum");
    result = exchange(commands, core->Shutdown, 4, "");
    require(result.status == core->Ok, "shutdown allowed at maximum");
    commands = core->start();
    result = exchange(commands, core->Open, 0, "a");
    result = exchange(commands, core->Replace, 1, "b");
    result = exchange(commands, core->Replace, 2, "c");
    result = exchange(commands, core->Close, 3, "");
    require(result.status == core->Ok && result.revision == 4 && !result.opened, "closed at maximum");
    result = exchange(commands, core->Open, 4, "d");
    require(result.status == core->RevisionLimit && !result.opened && result.text == "",
            "reopen cannot wrap or revive a retired session");
    result = exchange(commands, core->Shutdown, 4, "");
    require(result.status == core->Ok, "stop closed owner at maximum");
    sys->print("PASS idecore revision %d checks\n", checks);
    halt();
}
