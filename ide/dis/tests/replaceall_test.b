implement ReplaceAllTest;
include "sys.m";
sys: Sys;
include "idecore.m";
core: Idecore;


# Host bootstrap only; the service itself uses no Sys or host UI access.
ReplaceAllTest: module
{
    init: fn();
};
checks := 0;


# End this isolated reference-emulator process without a shell or window server.
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


# Fail loudly in release builds as well as development configurations.
require(ok: int, message: string)
{
    if(!ok)
    {
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}


# Retain a reply rendezvous until this serialized operation completes.
exchange(commands: chan of core->Request, operation, revision, first, last: int,
needle, replacement: string): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(31, operation, revision, first, last, needle, replacement, reply);
    return <-reply;
}


# Match count is meaningful only on successful ReplaceAll; errors preserve every field.
state(result: core->Reply, status, revision, saved, matches: int, text: string)
{
    require(result.id == 31 && result.status == status, "identity/status");
    require(result.revision == revision && result.saved == saved && result.opened, "revision/saved/session");
    require(result.matches == matches && result.text == text, "match count and text");
    require(result.offset == -1 && result.line == -1 && result.column == -1, "no invented navigation position");
}


# Test scoped Unicode replacement, original-text matching and history as one transaction.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load service");
    commands := core->start();
    r := exchange(commands, core->ReplaceAll, 0, 0, 0, "", "");
    require(r.status == core->Closed && r.matches == -1, "closed precedes pattern validation");
    original := exchange(commands, core->Open, 0, 0, 0, "中😀-中😀-中😀", "");
    state(exchange(commands, core->ReplaceAll, 0, 0, 8, "中😀", "x"), core->Conflict, 1, 1, -1, original.text);
    state(exchange(commands, core->ReplaceAll, 1, -1, 8, "中😀", "x"), core->RangeError, 1, 1, -1, original.text);
    state(exchange(commands, core->ReplaceAll, 1, 0, 9, "中😀", "x"), core->RangeError, 1, 1, -1, original.text);
    state(exchange(commands, core->ReplaceAll, 1, 0, 8, "", "x"), core->Invalid, 1, 1, -1, original.text);
    state(exchange(commands, core->ReplaceAll, 1, 2, 4, "中😀", "x"), core->NotFound, 1, 1, -1, original.text);
    state(exchange(commands, core->ReplaceAll, 1, 0, 0, "中", "x"), core->NotFound, 1, 1, -1, original.text);
    commands <-= core->Request(99, core->ReplaceAll, 1, 0, 8, "中", "x", nil);
    state(exchange(commands, core->Snapshot, -1, 0, 0, "", ""), core->Ok, 1, 1, -1, original.text);
    # Only the middle pair is entirely inside [2,6); inserted text is not rescanned.
    r = exchange(commands, core->ReplaceAll, 1, 2, 6, "中😀", "中😀中😀");
    state(r, core->Ok, 2, 1, 1, "中😀-中😀中😀-中😀");
    require(original.text == "中😀-中😀-中😀" && original.revision == 1, "old reply retains immutable snapshot");
    r = exchange(commands, core->Undo, 2, 0, 0, "", "");
    state(r, core->Ok, 3, 1, -1, original.text);
    # Successful no-op must preserve the pending Redo and the current revision.
    state(exchange(commands, core->ReplaceAll, 3, 0, 8, "中😀", "中😀"), core->Ok, 3, 1, 3, original.text);
    r = exchange(commands, core->Redo, 3, 0, 0, "", "");
    state(r, core->Ok, 4, 1, -1, "中😀-中😀中😀-中😀");
    r = exchange(commands, core->Undo, 4, 0, 0, "", "");
    r = exchange(commands, core->ReplaceAll, 5, 0, 8, "中😀", "");
    state(r, core->Ok, 6, 1, 3, "--");
    state(exchange(commands, core->Redo, 6, 0, 0, "", ""), core->NotFound, 6, 1, -1, "--");
    r = exchange(commands, core->Undo, 6, 0, 0, "", "");
    state(r, core->Ok, 7, 1, -1, original.text);
    r = exchange(commands, core->Shutdown, 7, 0, 0, "", "");

    commands = core->start();
    r = exchange(commands, core->Open, 0, 0, 0, "aaaaa", "");
    state(exchange(commands, core->ReplaceAll, 1, 0, 5, "aa", "x"), core->Ok, 2, 1, 2, "xxa");
    state(exchange(commands, core->Undo, 2, 0, 0, "", ""), core->Ok, 3, 1, -1, "aaaaa");
    r = exchange(commands, core->Shutdown, 3, 0, 0, "", "");

    # Many replacements exercise balanced rendering and one-step Undo. The source
    # is ASCII only to keep fixture construction separate from Unicode service behavior.
    commands = core->start();
    data := array[32768] of byte;
    for(index := 0; index < len data; index++)
    {
        data[index] = byte 'a';
    }
    longtext := string data;
    r = exchange(commands, core->Open, 0, 0, 0, longtext, "");
    r = exchange(commands, core->ReplaceAll, 1, 0, len longtext, "a", "bb");
    require(r.status == core->Ok && r.matches == 32768 && len r.text == 65536 &&
    r.text[0] == 'b' && r.text[65535] == 'b', "many matches render without quadratic append");
    r = exchange(commands, core->Undo, r.revision, 0, 0, "", "");
    require(r.status == core->Ok && r.text == longtext && r.revision == 3, "all matches undo together");
    r = exchange(commands, core->Shutdown, r.revision, 0, 0, "", "");

    # Reject expansion beyond the document limit before publishing text/history.
    commands = core->start();
    # Construct the exact-size fixture once, avoiding self-concatenation in the
    # older reference runtime after the high-allocation rendering workload.
    data = array[core->MaxCharacters] of byte;
    for(index = 0; index < len data; index++)
    {
        data[index] = byte 'x';
    }
    huge := string data;
    r = exchange(commands, core->Open, 0, 0, 0, huge, "");
    state(exchange(commands, core->ReplaceAll, 1, 0, len huge, "x", "xx"),
    core->TooLarge, 1, 1, -1, huge);
    r = exchange(commands, core->Undo, 1, 0, 0, "", "");
    require(r.status == core->NotFound && r.text == huge, "failed bulk edit creates no history");
    r = exchange(commands, core->ReplaceAll, 1, 0, len huge, "xx", "x");
    require(r.status == core->Ok && r.matches == 524288 && len r.text == 524288, "large shrink is admitted");
    r = exchange(commands, core->Shutdown, r.revision, 0, 0, "", "");
    sys->print("PASS idecore replaceall %d checks\n", checks);
    halt();
}
