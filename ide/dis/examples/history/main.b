implement HistoryExample;
include "sys.m";
    sys: Sys;
include "idecore.m";
    core: Idecore;

# This developer bootstrap supplies init/reporting; idecore.dis is the library.
# Sys is used only to report and halt this example's private reference emulator.
HistoryExample: module
{
    init: fn();
};
checks := 0;

# Halt this isolated test process; the product core never controls its host.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if(control == nil)
        raise "fail: missing halt control";
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: halt returned";
}

# Check each reply before issuing the next revision-dependent request.
require(ok: int, message: string)
{
    if(!ok){
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}

# Keep the request/reply rendezvous paired. Coordinates are unused for history;
# Replace receives the current snapshot's complete half-open character range.
exchange(commands: chan of core->Request, operation, expected, length: int,
         text: string): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(1, operation, expected, 0, length, text, "", reply);
    return <-reply;
}

# Demonstrate a branch after Undo without reviving stale revision identities.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load core");
    commands := core->start();
    # Open is acknowledged clean at revision 1. Keep this reply to demonstrate
    # that later mutation never changes a previously delivered text snapshot.
    original := exchange(commands, core->Open, 0, 0, "first");
    require(original.status == core->Ok && original.revision == 1, "open first");
    edited := exchange(commands, core->Replace, 1, len original.text, "second");
    require(edited.status == core->Ok && edited.revision == 2, "edit second");
    # Undo restores spelling, not identity: first now has revision 3, saved 1.
    undone := exchange(commands, core->Undo, 2, 0, "");
    require(undone.status == core->Ok && undone.text == "first" && undone.revision == 3,
            "undo as new revision");
    require(undone.saved == 1, "history does not acknowledge disk save");
    sys->print("undo text=%s revision=%d saved=%d\n", undone.text, undone.revision, undone.saved);
    stale := exchange(commands, core->Redo, 2, 0, "");
    require(stale.status == core->Conflict && stale.revision == 3, "reject stale redo");
    redone := exchange(commands, core->Redo, 3, 0, "");
    require(redone.status == core->Ok && redone.text == "second" && redone.revision == 4,
            "redo as new revision");
    undone = exchange(commands, core->Undo, 4, 0, "");
    require(undone.status == core->Ok && undone.revision == 5, "undo before branch");
    branch := exchange(commands, core->Replace, 5, len undone.text, "third");
    require(branch.status == core->Ok && branch.revision == 6 && branch.text == "third",
            "new edit clears redo");
    absent := exchange(commands, core->Redo, 6, 0, "");
    require(absent.status == core->NotFound && absent.text == "third", "redo branch retired");
    require(original.text == "first" && original.revision == 1, "retained original snapshot");
    sys->print("branch text=%s revision=%d saved=%d\n", branch.text, branch.revision, branch.saved);
    stopped := exchange(commands, core->Shutdown, 6, 0, "");
    require(stopped.status == core->Ok, "terminal acknowledgement");
    # No producer may send again after the terminal Shutdown acknowledgement.
    sys->print("PASS idecore historyexample %d checks\n", checks);
    halt();
}
