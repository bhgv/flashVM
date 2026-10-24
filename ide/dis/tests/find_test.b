implement FindTest;
include "sys.m";
    sys: Sys;
include "idecore.m";
    core: Idecore;

# Test-only bootstrap owns its emulator; production imports no Sys or UI modules.
FindTest: module
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
exchange(commands: chan of core->Request, operation, expected, first, last: int, text: string): core->Reply
{
    reply := chan of core->Reply;
    commands <-= core->Request(42, operation, expected, first, last, text, "", reply);
    result := <-reply;
    require(result.id == 42, "correlation");
    return result;
}

# Compare the complete state and first exact candidate against explicit expectations.
search(text, needle: string, first, last, status, offset, line, column: int)
{
    commands := core->start();
    result := exchange(commands, core->Open, 0, 0, 0, text);
    require(result.status == core->Ok, "open");
    result = exchange(commands, core->Find, 1, first, last, needle);
    require(result.status == status && result.offset == offset &&
            result.line == line && result.column == column, "search result");
    require(result.text == text && result.revision == 1 && result.saved == 1 &&
            result.opened, "unchanged snapshot");
    result = exchange(commands, core->Shutdown, 1, 0, 0, "");
    require(result.status == core->Ok, "shutdown");
}

# Execute first-match, bounded-range, Unicode and adversarial prefix-fallback cases.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load product core");
    search("ababa", "aba", 0, 5, core->Ok, 0, 1, 0);
    search("ababa", "aba", 1, 5, core->Ok, 2, 1, 2);
    search("ababa", "aba", 0, 2, core->NotFound, -1, -1, -1);
    search("ababa", "ABA", 0, 5, core->NotFound, -1, -1, -1);
    search("ababa", "b", 2, 3, core->NotFound, -1, -1, -1);
    search("", "x", 0, 0, core->NotFound, -1, -1, -1);
    search("x", "x", 1, 1, core->NotFound, -1, -1, -1);
    search("x", "", 0, 1, core->Invalid, -1, -1, -1);
    search("x", "x", -1, 1, core->RangeError, -1, -1, -1);
    search("x", "x", 1, 0, core->RangeError, -1, -1, -1);
    search("x", "x", 0, 2, core->RangeError, -1, -1, -1);
    search("A中😀\nB中😀", "中😀", 2, 7, core->Ok, 5, 2, 1);
    search("a\r\nb", "\r\n", 0, 4, core->Ok, 1, 1, 1);
    search("a\nb", "\nb", 0, 3, core->Ok, 1, 1, 1);
    search("éé", "é", 0, 3, core->Ok, 2, 1, 2);
    search("aaaaaaab", "aaab", 0, 8, core->Ok, 4, 1, 4);
    # Build independent ASCII fixtures through byte arrays. The reference VM has
    # an unrelated addc allocator fault with shared, long concatenated operands.
    data := array[32769] of byte;
    for(index := 0; index < len data; index++)
        data[index] = byte 'a';
    data[len data - 1] = byte 'b';
    longtext := string data;
    needledata := array[16385] of byte;
    for(index = 0; index < len needledata; index++)
        needledata[index] = byte 'a';
    needledata[len needledata - 1] = byte 'b';
    longneedle := string needledata;
    search(longtext, longneedle, 0, 32769, core->Ok, 16384, 1, 16384);
    search(longtext[:32768], longneedle, 0, 32768, core->NotFound, -1, -1, -1);
    commands := core->start();
    result := exchange(commands, core->Find, 0, 0, 0, "x");
    require(result.status == core->Closed, "closed before pattern validation");
    result = exchange(commands, core->Open, 0, 0, 0, "abc");
    result = exchange(commands, core->Replace, 1, 0, 1, "x");
    result = exchange(commands, core->Find, 1, 0, 3, "x");
    require(result.status == core->Conflict && result.revision == 2 && result.text == "xbc" &&
            result.offset == -1, "stale search after edit");
    result = exchange(commands, core->Find, 2, 0, 3, "x");
    require(result.status == core->Ok && result.offset == 0 && result.saved == 1,
            "find preserves dirty state");
    result = exchange(commands, core->Shutdown, 2, 0, 0, "");
    require(result.status == core->Ok, "stop edited owner");
    sys->print("PASS idecore find %d checks\n", checks);
    halt();
}
