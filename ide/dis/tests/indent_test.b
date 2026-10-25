implement IndentTest;
include "sys.m";
include "uibridge.m";
include "idecore.m";
include "ideinput.m";

IndentTest: module
{
    init: fn(); # Exercise real Dis indent commands through the document owner.
};

core: Idecore; # Actual channel-owned document service.
policy: Ideinput; # Real input policy, shared with both editor frontends.
document: chan of Idecore->Request; # Borrowed by calls; shut down before exit.
state: ref Ideinput->State; # Exclusive test controller state.


# Send a synchronous value operation to the real document owner; returns its acknowledgement.
# operation/revision/text are values; blocks only this test process.
call(operation, revision: int, text: string): Idecore->Reply
{
    answers := chan of Idecore->Reply;
    document <-= Idecore->Request(1, operation, revision, 0, 0, text, "", answers);
    return <-answers;
}


# Apply a key transition and compare source, both scalar endpoints and actual handled state.
# All arguments are values; failure raises visibly and no native policy substitutes for Dis.
key(code, pressed, caret, anchor: int, text: string)
{
    (ok, result) := policy->apply(core, document, state,
        ref Uibridge->Message(Uibridge->Key, 0, code, pressed, 0, 0, ""));
    if (!ok || result == nil || !result.handled || result.caret != caret || result.anchor != anchor
        || result.document.text != text || result.document.status != core->Ok) {
        raise "fail: indent command";
    }
}


# Validate independent word boundaries, reversed selections, one-step Undo and CRLF pairs.
# No parameters; modules and owner remain rooted until acknowledged shutdown.
init()
{
    sys := load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    policy = load Ideinput Ideinput->PATH;
    if (sys == nil || core == nil || policy == nil) {
        raise "fail: indent test modules";
    }
    document = core->start();
    state = policy->create();
    text := "a";
    reply := call(core->Open, 0, text);
    policy->observe(state, reply);
    key(35, 1, 1, 1, text);
    key(9, 1, 4, 4, "a   ");
    key(17, 1, 4, 4, "a   ");
    key(90, 1, 1, 1, "a");
    key(17, 0, 1, 1, "a");
    reply = call(core->Close, state.revision, "");
    text = "α\r\n  β\nend";
    reply = call(core->Open, reply.revision, text);
    policy->observe(state, reply);
    if (!policy->select(state, reply, 0, 7)) {
        raise "fail: indent selection";
    }
    key(9, 1, 15, 0, "    α\r\n      β\nend");
    key(16, 1, 15, 0, "    α\r\n      β\nend");
    key(9, 1, 7, 0, text);
    key(16, 0, 7, 0, text);
    # Reverse selection retains direction after expanding to the affected full line.
    reply = call(core->Snapshot, 0, "");
    if (!policy->select(state, reply, 7, 8)) {
        raise "fail: indent reversed selection setup";
    }
    if (!policy->select(state, reply, 8, 7)) {
        raise "fail: indent reversed selection";
    }
    key(9, 1, 7, 14, "α\r\n  β\n    end");
    # Undo restores the entire block as a single source edit.
    key(17, 1, 7, 14, "α\r\n  β\n    end");
    key(90, 1, 7, 7, text);
    key(17, 0, 7, 7, text);
    reply = call(core->Close, state.revision, "");
    reply = call(core->Open, reply.revision, "\tword\n");
    policy->observe(state, reply);
    key(39, 1, 1, 1, "\tword\n");
    key(16, 1, 1, 1, "\tword\n");
    key(9, 1, 0, 0, "word\n");
    before := state.revision;
    key(9, 1, 0, 0, "word\n");
    if (state.revision != before) {
        raise "fail: no-op unindent changed history";
    }
    key(16, 0, 0, 0, "word\n");
    reply = call(core->Shutdown, state.revision, "");
    if (reply.status != core->Ok) {
        raise "fail: indent test shutdown";
    }
    sys->print("PASS Dis Tab stops, block indentation, reversed selections, CRLF, Undo and no-op unindent\n");
}
