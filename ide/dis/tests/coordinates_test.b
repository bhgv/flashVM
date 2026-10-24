implement CoordinatesTest;
include "sys.m";
    sys: Sys;
include "idecore.m";
    core: Idecore;

# Test-only bootstrap owns its emulator; production imports no Sys or UI modules.
CoordinatesTest: module
{
    init: fn();
};

# Count assertions only after actual channel responses, never from a host model.
checks := 0;

# Stop only the private emulator selected by the regression runner.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if(control == nil)
        raise "fail: missing test halt control";
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: test halt returned";
}

# Emit a visible failure because some reference emulators return zero after traps.
require(ok: int, message: string)
{
    if(!ok){
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}

# Complete one rendezvous with a private response channel and echoed correlation ID.
exchange(commands: chan of core->Request, operation, expected, first, last: int): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(42, operation, expected, first, last, "", reply);
    result := <-reply;
    require(result.id == 42, "correlation");
    return result;
}

# Errors and noncoordinate operations must not accidentally expose stale coordinates.
absent(result: core->Reply, status: int)
{
    require(result.status == status, "error status");
    require(result.offset == -1 && result.line == -1 && result.column == -1,
            "coordinate sentinel");
}

# Every character boundary, including EOF and LF, has an exact inverse mapping.
roundtrip(text: string)
{
    commands := core->start();
    reply := chan of core->Reply;
    commands <-= core->Request(0, core->Open, 0, 0, 0, text, reply);
    absent(<-reply, core->Ok);
    line := 1;
    column := 0;
    for(offset := 0; offset <= len text; offset++){
        located := exchange(commands, core->Locate, 1, offset, 999);
        require(located.status == core->Ok && located.offset == offset &&
                located.line == line && located.column == column, "authored coordinate");
        require(located.opened && located.revision == 1 && located.saved == 1 &&
                located.text == text, "query preserves snapshot");
        converted := exchange(commands, core->Offset, 1, line, column);
        require(converted.status == core->Ok && converted.offset == offset &&
                converted.line == line && converted.column == column, "inverse coordinate");
        if(offset < len text){
            if(text[offset] == '\n'){
                line++;
                column = 0;
            }else
                column++;
        }
    }
    absent(exchange(commands, core->Locate, 0, 0, 0), core->Conflict);
    absent(exchange(commands, core->Locate, 1, -1, 0), core->RangeError);
    absent(exchange(commands, core->Locate, 1, len text + 1, 0), core->RangeError);
    absent(exchange(commands, core->Offset, 1, 0, 0), core->RangeError);
    absent(exchange(commands, core->Offset, 1, 1, -1), core->RangeError);
    absent(exchange(commands, core->Offset, 1, 2147483647, 0), core->RangeError);
    absent(exchange(commands, core->Offset, 1, 1, 2147483647), core->RangeError);
    # A nil reply cannot mutate the document or stop the service.
    commands <-= core->Request(0, core->Locate, 1, 0, 0, "", nil);
    absent(exchange(commands, core->Snapshot, -1, 0, 0), core->Ok);
    absent(exchange(commands, core->Close, 1, 0, 0), core->Ok);
    absent(exchange(commands, core->Locate, 2, 0, 0), core->Closed);
    absent(exchange(commands, core->Offset, 2, 1, 0), core->Closed);
    absent(exchange(commands, core->Shutdown, 2, 0, 0), core->Ok);
}

# Cover Unicode character units, empty/trailing lines and unnormalized CRLF text.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load product core");
    roundtrip("");
    roundtrip("abc");
    roundtrip("\n");
    roundtrip("\n\n");
    roundtrip("A中😀é\nZ\n");
    roundtrip("A\r\nB\rC");
    roundtrip("a\nbc");
    # Reject crossing an LF into the following line through a large column.
    commands := core->start();
    reply := chan of core->Reply;
    commands <-= core->Request(0, core->Open, 0, 0, 0, "a\nbc", reply);
    absent(<-reply, core->Ok);
    absent(exchange(commands, core->Offset, 1, 1, 2), core->RangeError);
    absent(exchange(commands, core->Shutdown, 1, 0, 0), core->Ok);
    sys->print("PASS idecore coordinates %d checks\n", checks);
    halt();
}
