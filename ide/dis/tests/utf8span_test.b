implement Utf8SpanTest;
include "sys.m";
sys: Sys;
include "idecore.m";
core: Idecore;


# Isolated reference-VM driver: Sys is never imported by the production service.
Utf8SpanTest: module
{
    init: fn();
};
checks := 0;


# End only this explicitly created test emulator.
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


# Preserve assertion behavior in every native build configuration.
require(ok: int, message: string)
{
    if(!ok)
    {
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}


# Correlate one channel transaction and wait for its complete reply.
exchange(commands: chan of core->Request, operation, revision, first, last: int,
text: string): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(9, operation, revision, first, last, text, "", reply);
    return <-reply;
}


# Every span is read-only and independent of unrelated needle/replacement fields.
span(commands: chan of core->Request, first, last, status, offset, end, line, column: int)
{
    r := exchange(commands, core->Utf8Span, 1, first, last, "ignored");
    require(r.status == status && r.id == 9, "status/identity");
    require(r.revision == 1 && r.saved == 1 && r.opened, "read-only snapshot");
    require(r.offset == offset && r.selection_end == end && r.line == line && r.column == column,
    "exact character selection/coordinates");
    require(r.matches == -1, "no replacement count");
}


# Exercise UTF-8 producer byte ranges without treating bytes as Limbo indices.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load service");
    commands := core->start();
    r := exchange(commands, core->Utf8Span, 0, 0, 0, "");
    require(r.status == core->Closed && r.selection_end == -1, "closed before range conversion");
    # BOM(3), A(1), 中(3), emoji(4), CR(1), LF(1), e(1), combining acute(2):
    # byte length 16, eight characters, last line has two separate characters.
    source := "\uFEFFA中😀\r\ne\u0301";
    r = exchange(commands, core->Open, 0, 0, 0, source);
    require(r.status == core->Ok && len r.text == 8, "source fixture units");
    span(commands, 0, 0, core->Ok, 0, 0, 1, 0);
    span(commands, 0, 3, core->Ok, 0, 1, 1, 0);
    span(commands, 4, 7, core->Ok, 2, 3, 1, 2);
    span(commands, 7, 11, core->Ok, 3, 4, 1, 3);
    span(commands, 12, 13, core->Ok, 5, 6, 1, 5);  # LF begins at char 5, before line increment.
    span(commands, 13, 16, core->Ok, 6, 8, 2, 0);
    span(commands, 14, 16, core->Ok, 7, 8, 2, 1);
    span(commands, 16, 16, core->Ok, 8, 8, 2, 2);
    span(commands, 5, 7, core->RangeError, -1, -1, -1, -1);
    span(commands, 4, 6, core->RangeError, -1, -1, -1, -1);
    span(commands, 8, 11, core->RangeError, -1, -1, -1, -1);
    span(commands, 0, 17, core->RangeError, -1, -1, -1, -1);
    span(commands, -1, 0, core->RangeError, -1, -1, -1, -1);
    span(commands, 7, 4, core->RangeError, -1, -1, -1, -1);
    r = exchange(commands, core->Replace, 1, 1, 2, "B");
    stale := exchange(commands, core->Utf8Span, 1, 4, 7, "");
    require(stale.status == core->Conflict && stale.offset == -1 && stale.selection_end == -1,
    "compiler result tied to old source revision is rejected");
    r = exchange(commands, core->Shutdown, r.revision, 0, 0, "");

    commands = core->start();
    r = exchange(commands, core->Open, 0, 0, 0, "");
    span(commands, 0, 0, core->Ok, 0, 0, 1, 0);
    r = exchange(commands, core->Shutdown, 1, 0, 0, "");
    # Limbo can hold non-Unicode character values; reject the entire snapshot
    # even when the requested point precedes the invalid character.
    commands = core->start();
    invalid := "xx";
    invalid[1] = 16rd800;
    r = exchange(commands, core->Open, 0, 0, 0, invalid);
    span(commands, 0, 0, core->Invalid, -1, -1, -1, -1);
    r = exchange(commands, core->Shutdown, 1, 0, 0, "");
    commands = core->start();
    invalid[1] = 16r110000;
    r = exchange(commands, core->Open, 0, 0, 0, invalid);
    span(commands, 0, 0, core->Invalid, -1, -1, -1, -1);
    r = exchange(commands, core->Shutdown, 1, 0, 0, "");
    commands = core->start();
    maximum := "x";
    maximum[0] = 16r10ffff;
    r = exchange(commands, core->Open, 0, 0, 0, maximum);
    span(commands, 0, 4, core->Ok, 0, 1, 1, 0);
    r = exchange(commands, core->Shutdown, 1, 0, 0, "");
    sys->print("PASS idecore utf8span %d checks\n", checks);
    halt();
}
