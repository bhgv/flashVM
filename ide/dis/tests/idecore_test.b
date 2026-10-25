implement IdecoreTest;
include "sys.m";
sys: Sys;
include "idecore.m";
core: Idecore;


# Test bootstrap has no graphics context; it runs directly as an emulator init module.
IdecoreTest: module
{
    init: fn();
};

# Each test process owns one monotonic correlation sequence and an assertion count.
nextid := 1;
checks := 0;


# Stop this explicitly selected test emulator; this host-only control is absent from idecore.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if(control == nil)
    {
        raise "fail: cannot open test emulator halt control";
    }
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: test emulator halt returned";
}


# Make failures visible even on runtimes that return success after a root-process exception.
require(ok: int, message: string)
{
    if(!ok)
    {
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}


# Correlate one synchronous command; callers own the reply rendezvous until completion.
exchange(commands: chan of core->Request, operation, expected, first, last: int,
text: string): core->Reply
{
    reply := chan of core->Reply;
    id := nextid++;
    commands <-= core->Request(id, operation, expected, first, last, text, "", reply);
    result := <-reply;
    require(result.id == id, "correlation ID");
    return result;
}


# Compare every observable field so rejected edits cannot silently change document state.
state(reply: core->Reply, status, opened, revision, saved: int, text: string)
{
    require(reply.status == status, "status");
    require(reply.opened == opened, "opened");
    require(reply.revision == revision, "revision");
    require(reply.saved == saved, "saved revision");
    require(reply.text == text, "text");
}


# Execute session, Unicode, invalid-operation, revision and request-backpressure contracts.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load idecore.dis");
    commands := core->start();
    state(exchange(commands, core->Snapshot, -1, 0, 0, ""), core->Ok, 0, 0, 0, "");
    state(exchange(commands, core->Replace, 0, 0, 0, "x"), core->Closed, 0, 0, 0, "");
    state(exchange(commands, core->Open, 0, 0, 0, "A中B"), core->Ok, 1, 1, 1, "A中B");
    state(exchange(commands, core->Open, 1, 0, 0, "other"), core->Invalid, 1, 1, 1, "A中B");
    state(exchange(commands, core->Replace, 0, 1, 2, "x"), core->Conflict, 1, 1, 1, "A中B");
    state(exchange(commands, core->Replace, 1, -1, 2, "x"), core->RangeError, 1, 1, 1, "A中B");
    state(exchange(commands, core->Replace, 1, 2, 1, "x"), core->RangeError, 1, 1, 1, "A中B");
    state(exchange(commands, core->Replace, 1, 0, 4, "x"), core->RangeError, 1, 1, 1, "A中B");
    state(exchange(commands, core->Replace, 1, 1, 2, "中"), core->Ok, 1, 1, 1, "A中B");
    state(exchange(commands, core->Replace, 1, 1, 2, "😀"), core->Ok, 1, 2, 1, "A😀B");
    state(exchange(commands, core->MarkSaved, 1, 0, 0, ""), core->Conflict, 1, 2, 1, "A😀B");
    state(exchange(commands, core->MarkSaved, 2, 0, 0, ""), core->Ok, 1, 2, 2, "A😀B");
    state(exchange(commands, 999, 2, 0, 0, ""), core->Unknown, 1, 2, 2, "A😀B");
    # A missing reply must neither modify text nor terminate the owner process.
    commands <-= core->Request(0, core->Shutdown, 2, 0, 0, "", "", nil);
    commands <-= core->Request(0, core->Replace, 2, 0, 3, "bad", "", nil);
    state(exchange(commands, core->Snapshot, -1, 0, 0, ""), core->Ok, 1, 2, 2, "A😀B");
    state(exchange(commands, core->Close, 2, 0, 0, ""), core->Ok, 0, 3, 3, "");
    state(exchange(commands, core->MarkSaved, 3, 0, 0, ""), core->Closed, 0, 3, 3, "");
    state(exchange(commands, core->Open, 2, 0, 0, "new"), core->Conflict, 0, 3, 3, "");
    state(exchange(commands, core->Open, 3, 0, 0, "new"), core->Ok, 1, 4, 4, "new");
    state(exchange(commands, core->Replace, 1, 0, 3, "old"), core->Conflict, 1, 4, 4, "new");
    state(exchange(commands, core->Replace, 4, 0, 0, "start-"), core->Ok, 1, 5, 4, "start-new");
    state(exchange(commands, core->Replace, 5, 9, 9, "-end"), core->Ok, 1, 6, 4, "start-new-end");
    state(exchange(commands, core->Replace, 6, 0, 13, ""), core->Ok, 1, 7, 4, "");
    state(exchange(commands, core->Shutdown, 6, 0, 0, ""), core->Conflict, 1, 7, 4, "");
    state(exchange(commands, core->Shutdown, 7, 0, 0, ""), core->Ok, 1, 7, 4, "");
    sys->print("PASS idecore %d checks\n", checks);
    halt();
}
