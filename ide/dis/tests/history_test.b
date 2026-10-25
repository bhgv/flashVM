implement HistoryTest;
include "sys.m";
sys: Sys;
include "idecore.m";
core: Idecore;


# Bootstrap only: the production service never imports Sys or halts its host.
HistoryTest: module
{
    init: fn();
};
checks := 0;


# Terminate this isolated reference emulator after a terminal test result.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if(control == nil)
    {
        raise "fail: missing halt control";
    }
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: halt returned";
}


# Keep assertions active regardless of native build configuration.
require(ok: int, message: string)
{
    if(!ok)
    {
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}


# Own one reply rendezvous until the serialized request completes.
exchange(commands: chan of core->Request, operation, expected, first, last: int,
text: string): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(7, operation, expected, first, last, text, "", reply);
    return <-reply;
}


# Rejected history commands must retain the entire observable snapshot.
state(result: core->Reply, status, revision, saved: int, text: string)
{
    require(result.status == status, "status");
    require(result.revision == revision && result.saved == saved, "revision and saved");
    require(result.text == text, "text");
    require(result.id == 7, "correlation");
    require(result.offset == -1 && result.line == -1 && result.column == -1, "no invented navigation result");
}


# Exercise history semantics, admission, session retirement and exact entry eviction.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load core");
    require(core->MaxHistoryEntries == 64, "production entry limit");
    commands := core->start();
    state(exchange(commands, core->Undo, 0, 0, 0, ""), core->Closed, 0, 0, "");
    original := exchange(commands, core->Open, 0, 0, 0, "A中😀");
    state(original, core->Ok, 1, 1, "A中😀");
    state(exchange(commands, core->Undo, 1, 0, 0, ""), core->NotFound, 1, 1, "A中😀");
    state(exchange(commands, core->Replace, 1, 0, 3, "B"), core->Ok, 2, 1, "B");
    state(exchange(commands, core->Undo, 2, 0, 0, ""), core->Ok, 3, 1, "A中😀");
    state(exchange(commands, core->Redo, 2, 0, 0, ""), core->Conflict, 3, 1, "A中😀");
    # A nil destination is ignored, including for a history-changing command.
    commands <-= core->Request(0, core->Redo, 3, 0, 0, "", "", nil);
    state(exchange(commands, core->Snapshot, 0, 0, 0, ""), core->Ok, 3, 1, "A中😀");
    state(exchange(commands, core->Replace, 3, 0, 3, "A中😀"), core->Ok, 3, 1, "A中😀");
    state(exchange(commands, core->Replace, 3, -1, 1, ""), core->RangeError, 3, 1, "A中😀");
    state(exchange(commands, core->MarkSaved, 3, 0, 0, ""), core->Ok, 3, 3, "A中😀");
    state(exchange(commands, core->Redo, 3, 0, 0, ""), core->Ok, 4, 3, "B");
    state(exchange(commands, core->Undo, 4, 0, 0, ""), core->Ok, 5, 3, "A中😀");
    state(exchange(commands, core->Replace, 5, 0, 3, "branch"), core->Ok, 6, 3, "branch");
    state(exchange(commands, core->Redo, 6, 0, 0, ""), core->NotFound, 6, 3, "branch");
    state(exchange(commands, core->Undo, 6, 0, 0, ""), core->Ok, 7, 3, "A中😀");
    state(exchange(commands, core->Redo, 7, 0, 0, ""), core->Ok, 8, 3, "branch");
    require(original.text == "A中😀" && original.revision == 1, "retained Reply remains a snapshot");
    state(exchange(commands, core->Close, 8, 0, 0, ""), core->Ok, 9, 9, "");
    state(exchange(commands, core->Open, 9, 0, 0, "fresh"), core->Ok, 10, 10, "fresh");
    state(exchange(commands, core->Undo, 10, 0, 0, ""), core->NotFound, 10, 10, "fresh");
    state(exchange(commands, core->Redo, 10, 0, 0, ""), core->NotFound, 10, 10, "fresh");
    state(exchange(commands, core->Shutdown, 10, 0, 0, ""), core->Ok, 10, 10, "fresh");

    # 65 changes admit exactly the most recent 64 prior texts; "0" is evicted.
    commands = core->start();
    result := exchange(commands, core->Open, 0, 0, 0, "0");
    for(index := 1; index <= 65; index++)
    {
        text := sys->sprint("%d", index);
        result = exchange(commands, core->Replace, result.revision, 0, len result.text, text);
        state(result, core->Ok, index + 1, 1, text);
    }
    revision := result.revision;
    for(index = 64; index >= 1; index--)
    {
        revision++;
        result = exchange(commands, core->Undo, result.revision, 0, 0, "");
        state(result, core->Ok, revision, 1, sys->sprint("%d", index));
    }
    state(exchange(commands, core->Undo, revision, 0, 0, ""), core->NotFound, revision, 1, "1");
    for(index = 2; index <= 65; index++)
    {
        revision++;
        result = exchange(commands, core->Redo, result.revision, 0, 0, "");
        state(result, core->Ok, revision, 1, sys->sprint("%d", index));
    }
    state(exchange(commands, core->Redo, revision, 0, 0, ""), core->NotFound, revision, 1, "65");
    state(exchange(commands, core->Shutdown, revision, 0, 0, ""), core->Ok, revision, 1, "65");
    sys->print("PASS idecore history %d checks\n", checks);
    halt();
}
