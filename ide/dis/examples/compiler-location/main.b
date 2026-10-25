implement CompilerLocationExample;
include "sys.m";
sys: Sys;
include "diagnostic-values.m";
include "idecore.m";
core: Idecore;


# This developer bootstrap supplies init/reporting; idecore.dis is the library.
# Sys is used only to report and halt this example's private reference emulator.
CompilerLocationExample: module
{
    init: fn();
};
checks := 0;


# Halt this isolated test process; the product core never controls its host.
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


# Check each reply before issuing the next revision-dependent request.
require(ok: int, message: string)
{
    if(!ok)
    {
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}


# Keep every read/edit on the service channel. The compiler's original source
# snapshot and its revision must be retained until the diagnostic arrives.
exchange(commands: chan of core->Request, operation, expected, first, last: int,
text: string): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(5, operation, expected, first, last, text, "", reply);
    return <-reply;
}


# The native compiler belongs to development tooling. This client itself is
# portable Limbo bytecode and receives only source bytes/range metadata as values.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load document service");
    commands := core->start();
    original := exchange(commands, core->Open, 0, 0, 0, Source);
    require(original.status == core->Ok, "retain submitted source snapshot");
    # byte offset 18 is character offset 13 here: 中 and emoji occupy 3/4 UTF-8
    # bytes respectively. Passing bytes directly to Locate would select wrongly.
    location := exchange(commands, core->Utf8Span, original.revision, ByteFirst, ByteLast, "");
    require(location.status == core->Ok && location.offset == 13 && location.selection_end == 20,
    "convert the exact compiler span to character coordinates");
    require(location.text[location.offset:location.selection_end] == "missing",
    "select the actual diagnostic identifier");
    require(location.line == 1 && location.column == 13, "editor column counts characters");
    sys->print("compiler bytes=[%d,%d) editor characters=[%d,%d)\n",
    ByteFirst, ByteLast, location.offset, location.selection_end);
    changed := exchange(commands, core->Replace, original.revision, 0, 0, " ");
    require(changed.status == core->Ok && changed.revision == 2, "document changes while compiling");
    stale := exchange(commands, core->Utf8Span, original.revision, ByteFirst, ByteLast, "");
    require(stale.status == core->Conflict && stale.offset == -1 && stale.selection_end == -1,
    "old diagnostic must not select text in the new revision");
    stopped := exchange(commands, core->Shutdown, changed.revision, 0, 0, "");
    require(stopped.status == core->Ok, "terminal acknowledgement");
    sys->print("PASS idecore compilerlocation %d checks\n", checks);
    halt();
}
