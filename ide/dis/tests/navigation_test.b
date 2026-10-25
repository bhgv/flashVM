implement NavigationTest;
include "sys.m";
include "uibridge.m";
include "idecore.m";
include "ideinput.m";

NavigationTest: module
{
    init: fn(); # Run real channel-owned logical-line navigation tests.
};

core: Idecore;                 # Actual document/history owner implementation.
policy: Ideinput;              # Actual Dis input policy under test.
document: chan of Idecore->Request; # Test-owned document service, shut down at the end.
state: ref Ideinput->State;    # Exclusive controller state; no native input replacement.


# Execute one synchronous owner operation; values are copied at rendezvous.
# Returns the acknowledged snapshot; blocks only this test's Dis process.
call(operation, revision: int, text: string): Idecore->Reply
{
    replies := chan of Idecore->Reply;
    document <-= Idecore->Request(1, operation, revision, 0, 0, text, "", replies);
    return <-replies;
}


# Apply one key transition and check independently expected scalar positions.
# code/pressed/expected/anchor are values. Failure raises; success changes no document text.
key(code, pressed, expected, anchor: int)
{
    event := ref Uibridge->Message(Uibridge->Key, 0, code, pressed, 0, 0, "");
    (ok, result) := policy->apply(core, document, state, event);
    if (!ok || result == nil || result.caret != expected || result.anchor != anchor
        || result.document.revision != state.revision) {
        raise "fail: navigation key/selection";
    }
}


# Exercise uneven Unicode lines, retained goals, paging, CRLF and external revisions.
# No parameters; shuts down the actual document service before reporting success.
init()
{
    sys := load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    policy = load Ideinput Ideinput->PATH;
    if (sys == nil || core == nil || policy == nil) {
        raise "fail: navigation modules";
    }
    document = core->start();
    state = policy->create();
    reply := call(core->Open, 0, "abcd\nx\nЯ😀xyz\n");
    policy->observe(state, reply);
    key(35, 1, 4, 4);
    key(40, 1, 6, 6);
    key(40, 1, 11, 11);
    key(38, 1, 6, 6);
    key(38, 1, 4, 4);
    key(16, 1, 4, 4);
    key(40, 1, 6, 4);
    key(38, 1, 4, 4);
    key(16, 0, 4, 4);
    key(34, 1, 13, 13);
    key(33, 1, 4, 4);
    key(17, 1, 4, 4);
    key(35, 1, 13, 13);
    key(36, 1, 0, 0);
    key(17, 0, 0, 0);
    key(35, 1, 4, 4);
    key(40, 1, 6, 6);
    key(37, 1, 5, 5);
    key(40, 1, 7, 7);
    if (state.preferred != 0) {
        raise "fail: horizontal movement did not reset preferred column";
    }
    reply = call(core->Snapshot, 0, "");
    if (reply.revision != 1 || reply.text != "abcd\nx\nЯ😀xyz\n") {
        raise "fail: navigation changed document/history";
    }
    reply = call(core->Close, reply.revision, "");
    reply = call(core->Open, reply.revision, "abcd\r\nx\r\nabcd");
    policy->observe(state, reply);
    key(35, 1, 4, 4);
    key(40, 1, 7, 7);
    key(40, 1, 13, 13);
    # An independently acknowledged edit invalidates the old controller coordinates.
    reply = call(core->Replace, reply.revision, "new");
    event := ref Uibridge->Message(Uibridge->Key, 0, 40, 1, 0, 0, "");
    (ok, result) := policy->apply(core, document, state, event);
    if (!ok || result.handled || result.caret != 0 || state.preferred != -1) {
        raise "fail: external revision reused stale navigation goal";
    }
    reply = call(core->Shutdown, result.document.revision, "");
    if (reply.status != core->Ok) {
        raise "fail: navigation shutdown";
    }
    sys->print("PASS Dis navigation: Unicode logical columns, pages, selection, CRLF and stale revisions\n");
}
