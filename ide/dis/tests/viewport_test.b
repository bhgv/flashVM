implement ViewportTest;
include "sys.m";
include "ideviewport.m";

ViewportTest: module
{
    init: fn(); # Execute bounded viewport logic as real Dis bytecode.
};


# Verify logical scrolling, reveal bounds and UTF-8 byte budgets independently of native rendering.
# No parameters; raises on failure and returns after reporting a complete successful run.
init()
{
    sys := load Sys Sys->PATH;
    viewport := load Ideviewport Ideviewport->PATH;
    if (sys == nil || viewport == nil) {
        raise "fail: viewport modules";
    }
    state := viewport->create();
    text := "";
    for (i := 0; i < 40; i++) {
        text += "a\n";
    }
    (first, last, line) := viewport->window(state, text, 0, 1);
    if (first != 0 || last != 48 || line != 0) {
        raise "fail: initial logical viewport";
    }
    viewport->scroll(state, text, 3);
    (first, last, line) = viewport->window(state, text, 0, 0);
    if (first != 6 || last != 54 || line != 3) {
        raise "fail: scrolling moved the caret or used byte/visual coordinates";
    }
    (first, last, line) = viewport->window(state, text, 0, 1);
    if (first != 0 || last != 48) {
        raise "fail: reveal earlier caret";
    }
    (first, last, line) = viewport->window(state, text, 80, 1);
    if (first != 34 || last != 80 || line != 17) {
        raise "fail: trailing logical line context";
    }
    viewport->scroll(state, text, -999999);
    (first, last, line) = viewport->window(state, text, 0, 0);
    if (first != 0 || line != 0) {
        raise "fail: bounded upward wheel";
    }
    text = "";
    for (i = 0; i < 3000; i++) {
        text += "Я😀";
    }
    state = viewport->create();
    (first, last, line) = viewport->window(state, text, 0, 0);
    bytes := array of byte text[first:last];
    if (first != 0 || len bytes > 4096 || last <= 0 || line != 0) {
        raise "fail: multi-byte viewport budget";
    }
    (first, last, line) = viewport->window(state, text, len text, 1);
    bytes = array of byte text[first:last];
    if (first <= 0 || last != len text || len bytes > 4096 || line != 0) {
        raise "fail: long-line Unicode suffix reveal";
    }
    viewport->scroll(state, text, 999999);
    (first, last, line) = viewport->window(state, text, 0, 0);
    if (first != 0 || line != 0) {
        raise "fail: invented final empty logical line";
    }
    sys->print("PASS Dis viewport: logical scroll/reveal, bounded deltas, Unicode bytes and long lines\n");
}
