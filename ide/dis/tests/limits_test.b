implement LimitsTest;
include "sys.m";
    sys: Sys;
include "idecore.m";
    core: Idecore;

# Direct emulator bootstrap for boundary and multi-client behavior, without graphics modules.
LimitsTest: module
{
    init: fn();
};

# A local assertion counter is printed only after every service has acknowledged shutdown.
checks := 0;

# Terminate the isolated test emulator; this operation is never part of the product module.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if(control == nil)
        raise "fail: missing halt control";
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: halt returned";
}

# Report a test failure before stopping, so process exit status alone cannot hide it.
require(ok: int, message: string)
{
    if(!ok){
        sys->print("FAIL %s\n", message);
        halt();
    }
    checks++;
}

# Make one typed request and retain its response channel through the rendezvous.
exchange(commands: chan of core->Request, id, operation, expected, first, last: int,
         text: string): core->Reply
{
    response := chan of core->Reply;
    commands <-= core->Request(id, operation, expected, first, last, text, response);
    return <-response;
}

# Competing producers use the same expected revision; exactly one edit can commit.
contender(commands: chan of core->Request, id: int, text: string, done: chan of core->Reply)
{
    result := exchange(commands, id, core->Replace, 1, 0, 1, text);
    done <-= result;
}

# Exercise exact character bounds, immutable channel values and serial conflict resolution.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    require(core != nil, "load core");
    commands := core->start();
    huge := "中";
    while(len huge < core->MaxCharacters)
        huge += huge;
    require(len huge == core->MaxCharacters, "Unicode character limit fixture");
    result := exchange(commands, 1, core->Open, 0, 0, 0, huge + "x");
    require(result.status == core->TooLarge && !result.opened && result.revision == 0,
            "oversize open preserves closed state");
    result = exchange(commands, 2, core->Open, 0, 0, 0, huge);
    require(result.status == core->Ok && len result.text == core->MaxCharacters,
            "exact limit accepted");
    result = exchange(commands, 3, core->Replace, 1, 0, 0, "x");
    require(result.status == core->TooLarge && result.revision == 1 && result.text == huge,
            "oversize insertion preserves content/revision");
    result = exchange(commands, 4, core->Replace, 1, 0, 1, "x");
    require(result.status == core->Ok && result.revision == 2 && len result.text == core->MaxCharacters,
            "replacement at limit uses surviving length");
    result = exchange(commands, 5, core->Close, 2, 0, 0, "");
    require(result.status == core->Ok && result.revision == 3 && result.text == "", "close limit document");
    result = exchange(commands, 6, core->Shutdown, 3, 0, 0, "");
    require(result.status == core->Ok, "stop limit owner");
    huge = "";

    commands = core->start();
    response := chan of core->Reply;
    request := core->Request(10, core->Open, 0, 0, 0, "A😀B", response);
    commands <-= request;
    # [GC:] The sent ADT is a channel value, not a shared mutable request reference.
    request.text = "changed after send";
    request.reply = nil;
    alt {
        commands <-= request =>
            require(0, "owner must wait for its first reply rendezvous");
        * =>
            require(1, "reply backpressure prevents another receive");
    }
    result = <-response;
    require(result.id == 10 && result.text == "A😀B", "sent value retained original fields");
    retained := result;
    result = exchange(commands, 11, core->Replace, 1, 2, 3, "C");
    require(result.text == "A😀C" && result.revision == 2, "supplementary character is one position");
    require(retained.text == "A😀B", "older response retains earlier text");
    result = exchange(commands, -1, core->Close, 2, 0, 0, "");
    require(result.status == core->Invalid && result.opened && result.revision == 2, "invalid ID has no effect");
    result = exchange(commands, 12, -1, 2, 0, 0, "");
    require(result.status == core->Unknown && result.revision == 2, "negative operation rejected");
    result = exchange(commands, 13, core->Shutdown, 2, 0, 0, "");
    require(result.status == core->Ok, "stop value owner");

    commands = core->start();
    result = exchange(commands, 20, core->Open, 0, 0, 0, "x");
    require(result.status == core->Ok, "open contested document");
    done := chan of core->Reply;
    spawn contender(commands, 21, "left", done);
    spawn contender(commands, 22, "right", done);
    first := <-done;
    second := <-done;
    require(first.id != second.id, "independent callers correlate their results");
    require((first.status == core->Ok && second.status == core->Conflict) ||
            (second.status == core->Ok && first.status == core->Conflict), "one commit and one conflict");
    require(first.revision == 2 && second.revision == 2, "both replies observe committed revision");
    require(first.text == second.text && (first.text == "left" || first.text == "right"),
            "no lost update or mixed document");
    result = exchange(commands, 23, core->Shutdown, 2, 0, 0, "");
    require(result.status == core->Ok, "stop contested owner");
    sys->print("PASS idecore limits %d checks\n", checks);
    halt();
}
