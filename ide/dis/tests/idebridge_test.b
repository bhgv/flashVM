implement IdebridgeTest;
include "sys.m";
include "uibridge.m";
include "idecore.m";
include "ideinput.m";
include "idebridge.m";

sys: Sys;                       # Reference-test diagnostics only, absent from application modules.
codec: Uibridge;                 # Real compiled common wire codec.
core: Idecore;                   # Real document owner, not a test replacement.
policy: Ideinput;                # Real Dis-owned editing policy.
bridge: Idebridge;               # Real protocol/application coordinator.
input: chan of array of byte;    # Real Dis request/input rendezvous.
output: chan of array of byte;   # Real Dis snapshot/reply rendezvous.
nextid := 1;                     # Monotonic explicit-command correlation for this test owner.
frames := 0;                     # Actual decoded frames validated by this run.


# Emulator init contract for this isolated reference fixture.
IdebridgeTest: module
{
    init: fn(); # No shell, graphical context or boot scripts are required.
};


# Halt only the explicitly selected reference test emulator; no product uses this control.
# No parameters. Returns never; failure raises a visible test exception.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if (control == nil) {
        raise "fail: no emulator halt control";
    }
    data := array of byte "halt";
    sys->write(control, data, len data);
    raise "fail: test halt returned";
}


# Report a concrete failed condition instead of relying on disabled assertions.
# message is borrowed diagnostic text. Returns never after stopping the test emulator.
fail(message: string)
{
    sys->print("FAIL idebridge %s\n", message);
    halt();
}


# Run the genuine application coordinator in its own Dis process.
# done is owned by the test caller. Returns nothing after publishing the final status.
application(done: chan of int)
{
    result := bridge->serveapp(codec, core, policy, input, output);
    done <-= result;
}


# Send one wire event/command, validate all UTF-8 snapshot fragments and consume selection.
# kind/code/revision/range/text are copied values; terminal omits post-shutdown selection.
# Returns the first snapshot frame, assembled text, caret, anchor and number of fragments.
exchange(kind, code, revision, first, last: int, text: string, terminal: int):
    (ref Uibridge->Message, string, int, int, int)
{
    id := 0;
    if (kind == codec->Request) {
        id = nextid++;
    }
    message := ref Uibridge->Message(kind, id, code, revision, first, last, text);
    (valid, frame) := codec->encode(message);
    if (!valid) {
        fail("request encode");
    }
    input <-= frame;
    initial: ref Uibridge->Message;
    contents := "";
    total := -1;
    offset := 0;
    parts := 0;
    for (;;) {
        response := <-output;
        (ok, value) := codec->decode(response);
        if (!ok) {
            fail("reply wire decode");
        }
        frames++;
        if (parts == 0) {
            initial = value;
        }
        expectedkind := codec->View;
        if (kind == codec->Request) {
            expectedkind = codec->Reply;
        }
        if (value.kind != expectedkind || value.id != id || value.flags != initial.flags
            || value.code != initial.code || value.first != initial.first || value.second != initial.second) {
            fail("snapshot identity changed between fragments");
        }
        bytes := array of byte value.text;
        if (len bytes < bridge->ReplyMeta || len bytes > codec->MaxText) {
            fail("snapshot fragment extent");
        }
        (hasstart, start) := codec->parseword(bytes, 40);
        (hastotal, size) := codec->parseword(bytes, 48);
        if (!hasstart || !hastotal || start != offset || size < 0 || (total >= 0 && size != total)) {
            fail("snapshot fragment sequence");
        }
        total = size;
        offset += len bytes - bridge->ReplyMeta;
        if (offset > total) {
            fail("snapshot fragment overflow");
        }
        contents += string bytes[bridge->ReplyMeta:];
        parts++;
        if (offset == total) {
            break;
        }
    }
    if (terminal) {
        return (initial, contents, -1, -1, parts);
    }
    selection := <-output;
    (ok, cursor) := codec->decode(selection);
    if (!ok || cursor.kind != codec->View || cursor.code != 65536 || cursor.id != 0
        || cursor.flags != initial.flags || cursor.first < 0 || cursor.second < 0
        || cursor.first > len contents || cursor.second > len contents) {
        fail("Dis-owned selection presentation");
    }
    frames++;
    return (initial, contents, cursor.first, cursor.second, parts);
}


# Test native-channel application composition, input policy, Unicode and large reply framing.
# No parameters. Prints PASS and halts this reference emulator after acknowledged shutdown.
init()
{
    sys = load Sys Sys->PATH;
    codec = load Uibridge Uibridge->PATH;
    core = load Idecore Idecore->PATH;
    policy = load Ideinput Ideinput->PATH;
    bridge = load Idebridge Idebridge->PATH;
    if (codec == nil || core == nil || policy == nil || bridge == nil) {
        fail("module loading");
    }
    input = chan of array of byte;
    output = chan of array of byte;
    done := chan of int;
    spawn application(done);
    (reply, text, caret, anchor, parts) := exchange(codec->Request, core->Open, 0, 0, 0, "аб😀\ncd", 0);
    if (reply.code != core->Ok || reply.flags != 1 || text != "аб😀\ncd" || caret != 0) {
        fail("open Unicode document");
    }
    (reply, text, caret, anchor, parts) = exchange(codec->Key, 35, 1, 0, 0, "", 0);
    if (caret != 3 || anchor != 3) {
        fail("End moves in Unicode characters");
    }
    (reply, text, caret, anchor, parts) = exchange(codec->Text, 0, 0, 0, 0, "X", 0);
    if (reply.flags != 2 || text != "аб😀X\ncd" || caret != 4) {
        fail("committed text inserted by Dis");
    }
    (reply, text, caret, anchor, parts) = exchange(codec->Key, 8, 1, 0, 0, "", 0);
    if (text != "аб😀\ncd" || caret != 3) {
        fail("Backspace handled by Dis");
    }
    exchange(codec->Key, 16, 1, 0, 0, "", 0);
    (reply, text, caret, anchor, parts) = exchange(codec->Key, 37, 1, 0, 0, "", 0);
    if (caret != 2 || anchor != 3) {
        fail("Shift selection spans one supplementary character");
    }
    (reply, text, caret, anchor, parts) = exchange(codec->Text, 0, 0, 0, 0, "Ю", 0);
    if (text != "абЮ\ncd" || caret != 3 || anchor != 3) {
        fail("selection replacement stays in Dis");
    }
    exchange(codec->Key, 16, 0, 0, 0, "", 0);
    exchange(codec->Key, 17, 1, 0, 0, "", 0);
    (reply, text, caret, anchor, parts) = exchange(codec->Key, 90, 1, 0, 0, "", 0);
    if (text != "аб😀\ncd") {
        fail("Ctrl+Z invokes real document history");
    }
    (reply, text, caret, anchor, parts) = exchange(codec->Key, 89, 1, 0, 0, "", 0);
    if (text != "абЮ\ncd") {
        fail("Ctrl+Y invokes real document history");
    }
    exchange(codec->Focus, 0, 0, 0, 0, "", 0);
    (reply, text, caret, anchor, parts) = exchange(codec->Key, 37, 1, 0, 0, "", 0);
    if (caret != 2) {
        fail("focus loss releases Control state");
    }
    revision := reply.flags;
    packed := codec->hexword(len array of byte "Ю") + "Ю" + "Я";
    (reply, text, caret, anchor, parts) = exchange(codec->Request, core->ReplaceAll, revision, 0, len text, packed, 0);
    if (text != "абЯ\ncd" || reply.code != core->Ok || caret != 0 || anchor != 0) {
        fail("typed ReplaceAll and Dis selection observation");
    }
    revision = reply.flags;
    large_text := "";
    for (i := 0; i < 966; i++) {
        large_text += "z";
    }
    large_text += "😀";
    for (i = 0; i < 54; i++) {
        large_text += "q";
    }
    (reply, text, caret, anchor, parts) = exchange(codec->Request, core->Replace, revision, 0, len text, large_text, 0);
    if (reply.code != core->Ok || text != large_text || parts != 2) {
        fail("large snapshot fragments preserve supplementary UTF-8 boundary");
    }
    revision = reply.flags;
    (reply, text, caret, anchor, parts) = exchange(codec->Request, core->Snapshot, 0, 0, 0, "", 0);
    if (reply.flags != revision || text != large_text || parts != 2) {
        fail("fragmented snapshot is stable");
    }
    (reply, text, caret, anchor, parts) = exchange(codec->Request, core->Shutdown, revision, 0, 0, "", 1);
    if (reply.code != core->Ok || <-done != 0) {
        fail("acknowledged application shutdown");
    }
    sys->print("PASS idebridge %d decoded frames; Dis editing, history, input and Unicode fragments\n", frames);
    halt();
}
