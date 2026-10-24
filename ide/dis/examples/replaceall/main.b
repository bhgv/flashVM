implement ReplaceAllExample;
include "sys.m";
    sys: Sys;
include "idecore.m";
    core: Idecore;

# This developer bootstrap supplies init/reporting; idecore.dis is the library.
# Sys is used only to report and halt this example's private reference emulator.
ReplaceAllExample: module
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

# Each command travels through the typed service channel. first/last are character
# offsets in the original snapshot; replacement is meaningful only for ReplaceAll.
exchange(commands: chan of core->Request, operation, expected, first, last: int,
         text, replacement: string): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(1, operation, expected, first, last, text, replacement, reply);
    return <-reply;
}

# A project-wide UI can reuse this operation per document, after checking each
# snapshot revision. This example owns just one document, with no file I/O.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load document service");
    commands := core->start();
    original := exchange(commands, core->Open, 0, 0, 0, "old中 old中 old中", "");
    require(original.status == core->Ok && original.revision == 1, "open source snapshot");
    # All three replacements inspect revision 1, then publish revision 2 together.
    # The replacement contains the needle: inserted text must never be searched.
    changed := exchange(commands, core->ReplaceAll, original.revision, 0,
                        len original.text, "old中", "old中新");
    require(changed.status == core->Ok && changed.matches == 3 && changed.revision == 2,
            "one transaction for three matches");
    require(changed.text == "old中新 old中新 old中新", "literal Unicode result");
    require(original.text == "old中 old中 old中", "retained reply is unchanged");
    sys->print("matches=%d revision=%d text=%s\n", changed.matches, changed.revision, changed.text);
    # A delayed dialog cannot overwrite the newer revision using its old selection.
    stale := exchange(commands, core->ReplaceAll, original.revision, 0,
                      len original.text, "old中", "lost edit");
    require(stale.status == core->Conflict && stale.text == changed.text, "stale dialog rejected");
    # Undo restores every replacement together under a fresh revision. saved stays
    # at the last disk acknowledgement; matching spelling alone is not MarkSaved.
    undone := exchange(commands, core->Undo, changed.revision, 0, 0, "", "");
    require(undone.status == core->Ok && undone.text == original.text && undone.revision == 3,
            "one Undo restores all three matches");
    require(undone.saved == 1, "history does not acknowledge a disk write");
    stopped := exchange(commands, core->Shutdown, undone.revision, 0, 0, "", "");
    require(stopped.status == core->Ok, "terminal acknowledgement");
    # The owner has now exited: sending another request would block forever.
    sys->print("PASS idecore replaceallexample %d checks\n", checks);
    halt();
}
