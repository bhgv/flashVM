implement WordsTest;
include "sys.m";
include "uibridge.m";
include "idecore.m";
include "ideinput.m";

WordsTest: module
{
    init: fn(); # Exercise real Dis word/CRLF commands through the document owner.
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
        raise "fail: word/CRLF command";
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
        raise "fail: word test modules";
    }
    document = core->start();
    state = policy->create();
    text := "alpha_2 ++ Я😀\tend";
    reply := call(core->Open, 0, text);
    policy->observe(state, reply);
    key(17, 1, 0, 0, text);
    key(39, 1, 8, 8, text);
    key(39, 1, 11, 11, text);
    key(16, 1, 11, 11, text);
    key(39, 1, 14, 11, text);
    key(37, 1, 11, 11, text);
    key(16, 0, 11, 11, text);
    key(46, 1, 11, 11, "alpha_2 ++ end");
    key(90, 1, 11, 11, text);
    key(8, 1, 8, 8, "alpha_2 Я😀\tend");
    key(90, 1, 8, 8, text);
    key(36, 1, 0, 0, text);
    key(8, 1, 0, 0, text);
    reply = call(core->Snapshot, 0, "");
    before := reply.revision;
    key(8, 1, 0, 0, text);
    reply = call(core->Snapshot, 0, "");
    if (reply.revision != before) {
        raise "fail: word boundary no-op made history";
    }
    reply = call(core->Close, reply.revision, "");
    reply = call(core->Open, reply.revision, "ab\r\ncd");
    policy->observe(state, reply);
    key(17, 0, 0, 0, "ab\r\ncd");
    key(35, 1, 2, 2, "ab\r\ncd");
    key(39, 1, 4, 4, "ab\r\ncd");
    key(37, 1, 2, 2, "ab\r\ncd");
    key(46, 1, 2, 2, "abcd");
    key(17, 1, 2, 2, "abcd");
    key(90, 1, 2, 2, "ab\r\ncd");
    key(17, 0, 2, 2, "ab\r\ncd");
    key(39, 1, 4, 4, "ab\r\ncd");
    key(8, 1, 2, 2, "abcd");
    reply = call(core->Shutdown, state.revision, "");
    if (reply.status != core->Ok) {
        raise "fail: word test shutdown";
    }
    sys->print("PASS Dis scalar word commands, selections, Undo, no-op history and CRLF pairs\n");
}
