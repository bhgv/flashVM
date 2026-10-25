implement ClipboardTest;
include "sys.m";
include "uibridge.m";
include "ideclipboard.m";

ClipboardTest: module
{
    init: fn(); # Verify real Dis clipboard staging without a native clipboard or document mutation.
};


# Build one decoded value fragment with an independently chosen offset/extent and stable ticket.
# codec is borrowed, body is copied into an immutable string; returns a new event value.
fragment(codec: Uibridge, ticket, revision, offset, total: int, body: string): ref Uibridge->Message
{
    return ref Uibridge->Message(Uibridge->Request, 1, 65557, revision, offset, total, codec->hexword(ticket) + body);
}


# Exercise complete publication, byte/scalar distinctions, private-prefix cancellation and all bounds.
# No parameters; errors raise visibly, with no host or document service required.
init()
{
    sys := load Sys Sys->PATH;
    codec := load Uibridge Uibridge->PATH;
    clipboard := load Ideclipboard Ideclipboard->PATH;
    if (sys == nil || codec == nil || clipboard == nil) {
        raise "fail: clipboard modules";
    }
    state := clipboard->create();
    if (!clipboard->begin(state, clipboard->Paste, 7, 3, 1, 4)) {
        raise "fail: clipboard begin";
    }
    (status, ready, text) := clipboard->chunk(codec, state, fragment(codec, 7, 3, 0, 9, "Я😀"));
    if (status != 0 || ready || text != "" || state.used != 6) {
        raise "fail: clipboard published a Unicode prefix";
    }
    (status, ready, text) = clipboard->chunk(codec, state, fragment(codec, 7, 3, 6, 9, "\nAB"));
    if (status != 0 || !ready || text != "Я😀\nAB" || state.action != clipboard->None || state.bytes != nil) {
        raise "fail: clipboard complete value/retirement";
    }
    # A stale original view/revision, repeated offset or changed extent cancels the entire prefix.
    for (mode := 0; mode < 5; mode++) {
        clipboard->begin(state, clipboard->Paste, 9, 4, 0, 0);
        (status, ready, text) = clipboard->chunk(codec, state, fragment(codec, 9, 4, 0, 2, "a"));
        event := fragment(codec, 9, 4, 1, 2, "b");
        if (mode == 0) {
            event.text = codec->hexword(8) + "b";
        } else if (mode == 1) {
            event.flags = 5;
        } else if (mode == 2) {
            event.first = 0;
        } else if (mode == 3) {
            event.second = 3;
        } else {
            event.text = codec->hexword(9) + "\0";
        }
        (status, ready, text) = clipboard->chunk(codec, state, event);
        if (status == 0 || ready || text != "" || state.bytes != nil || state.action != clipboard->None) {
            raise "fail: clipboard invalid continuation leaked its prefix";
        }
    }
    for (mode = 0; mode < 5; mode++) {
        clipboard->begin(state, clipboard->Paste, 10, 4, 0, 0);
        event := fragment(codec, 10, 4, 0, 1, "x");
        if (mode == 0) {
            event.second = clipboard->MaxBytes + 1;
        } else if (mode == 1) {
            event.first = -1;
        } else if (mode == 2) {
            event.text = codec->hexword(10);
        } else if (mode == 3) {
            event.text = "bad";
        } else {
            event.second = -1;
        }
        (status, ready, text) = clipboard->chunk(codec, state, event);
        if (status == 0 || ready || state.bytes != nil) {
            raise "fail: clipboard malformed first fragment allocated/published";
        }
    }
    clipboard->begin(state, clipboard->Paste, 11, 4, 0, 0);
    (status, ready, text) = clipboard->chunk(codec, state, fragment(codec, 11, 4, 0, 0, ""));
    if (status != 0 || !ready || text != "") {
        raise "fail: empty clipboard completion";
    }
    # An independently sized 64 KiB value crosses many envelope boundaries without concatenation work.
    block := "x";
    while (len block < 512) {
        block += block;
    }
    clipboard->begin(state, clipboard->Paste, 12, 4, 0, 0);
    for (offset := 0; offset < clipboard->MaxBytes; offset += 512) {
        (status, ready, text) = clipboard->chunk(codec, state, fragment(codec, 12, 4, offset, clipboard->MaxBytes, block));
        if (status != 0 || (ready && offset + 512 != clipboard->MaxBytes)) {
            raise "fail: bounded clipboard staging";
        }
    }
    if (!ready || len text != clipboard->MaxBytes || text[0] != 'x' || text[len text - 1] != 'x') {
        raise "fail: complete maximum clipboard";
    }
    sys->print("PASS Dis clipboard staging: Unicode bytes, exact completion, cancellation, stale/offset/NUL/size guards\n");
}
