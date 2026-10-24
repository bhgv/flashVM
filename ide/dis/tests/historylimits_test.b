implement HistoryLimitsTest;
include "sys.m";
    sys: Sys;
include "idecore.m";
    core: Idecore;

# Test-only headers reduce budgets; unchanged production source executes all guards.
HistoryLimitsTest: module
{
    init: fn();
};
checks := 0;

# Halt only the test emulator's namespace.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if(control == nil)
        raise "fail: missing halt control";
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: halt returned";
}

# Require a test invariant before proceeding to the next serialized exchange.
require(ok: int, message: string)
{
    if(!ok){
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}

# Replace the entire current text or travel using one expected revision.
exchange(commands: chan of core->Request, operation: int, previous: core->Reply,
         text: string): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(1, operation, previous.revision, 0, len previous.text, text, reply);
    return <-reply;
}

# Check status/text; successful history operations must advance monotonically.
state(result: core->Reply, status, revision: int, text: string)
{
    require(result.status == status, "status");
    require(result.revision == revision && result.saved == 1, "revision/saved");
    require(result.text == text, "text");
}

# Exercise both exact budgets, source-first eviction, empty and oversized snapshots.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil && core->MaxHistoryEntries == 3 && core->MaxHistoryCharacters == 6,
            "reduced history fixture");
    commands := core->start();
    empty := core->Reply(0, 0, 0, 0, 0, "", -1, -1, -1);
    r := exchange(commands, core->Open, empty, "a");
    r = exchange(commands, core->Replace, r, "bb");
    r = exchange(commands, core->Replace, r, "ccc");
    r = exchange(commands, core->Replace, r, "d");
    state(r, core->Ok, 4, "d");
    r = exchange(commands, core->Undo, r, "");
    state(r, core->Ok, 5, "ccc");
    r = exchange(commands, core->Redo, r, "");
    state(r, core->Ok, 6, "d");
    r = exchange(commands, core->Replace, r, "ee");
    state(r, core->Ok, 7, "ee");
    # Entry/character limits drop farthest "a". Undo must retain inverse "ee"
    # while evicting source's farthest "bb" to meet the six-character budget.
    r = exchange(commands, core->Undo, r, "");
    state(r, core->Ok, 8, "d");
    r = exchange(commands, core->Undo, r, "");
    state(r, core->Ok, 9, "ccc");
    r = exchange(commands, core->Undo, r, "");
    state(r, core->NotFound, 9, "ccc");
    r = exchange(commands, core->Redo, r, "");
    state(r, core->Ok, 10, "d");
    r = exchange(commands, core->Redo, r, "");
    state(r, core->Ok, 11, "ee");
    r = exchange(commands, core->Shutdown, r, "");

    # Empty snapshots consume entry budget even though they consume no characters.
    commands = core->start();
    r = exchange(commands, core->Open, empty, "");
    r = exchange(commands, core->Replace, r, "a");
    r = exchange(commands, core->Undo, r, "");
    state(r, core->Ok, 3, "");
    r = exchange(commands, core->Redo, r, "");
    state(r, core->Ok, 4, "a");
    # An edit succeeds even if its old text cannot fit the test history budget.
    r = exchange(commands, core->Replace, r, "1234567");
    r = exchange(commands, core->Replace, r, "x");
    state(r, core->Ok, 6, "x");
    r = exchange(commands, core->Undo, r, "");
    state(r, core->NotFound, 6, "x");
    r = exchange(commands, core->Shutdown, r, "");
    sys->print("PASS idecore historylimits %d checks\n", checks);
    halt();
}
