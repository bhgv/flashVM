implement FieldTest;
include "sys.m";
include "uibridge.m";
include "idecore.m";
include "ideinput.m";
include "idefield.m";

FieldTest: module
{
    init: fn(); # Verify single-line Dis value/selection policy independently of a native widget.
};

fields: Idefield; # Actual reusable implementation.
policy: Ideinput; # Shared scalar word-boundary policy, without a document owner.
state: ref Idefield->State; # [GC:] Exclusive field state and immutable text root for this test.


# Apply one independent expected field key transition; mismatch raises visibly.
# Arguments are values and the test owns state; no document or native operation occurs.
key(code, control, shift: int, text: string, caret, anchor: int)
{
    event := ref Uibridge->Message(Uibridge->Key, 0, code, 1, 0, 0, "");
    (status, handled) := fields->apply(policy, state, event, control, shift);
    if (status != fields->Ok || !handled || state.text != text || state.caret != caret || state.anchor != anchor) {
        raise "fail: field key/value/selection";
    }
}


# Exercise Unicode endpoints, reversed selections, words, no-ops and atomic rejected insertions.
# No arguments; all modules/state remain private to this test's Dis process.
init()
{
    sys := load Sys Sys->PATH;
    fields = load Idefield Idefield->PATH;
    policy = load Ideinput Ideinput->PATH;
    if (sys == nil || fields == nil || policy == nil) {
        raise "fail: field modules";
    }
    state = fields->create();
    if (fields->set(state, "Я😀abc", 0) != fields->Ok) {
        raise "fail: field set";
    }
    key(36, 0, 0, "Я😀abc", 0, 0);
    key(39, 0, 1, "Я😀abc", 1, 0);
    key(39, 0, 1, "Я😀abc", 2, 0);
    if (fields->insert(state, "Z") != fields->Ok || state.text != "Zabc" || state.caret != 1) {
        raise "fail: field Unicode selection insertion";
    }
    key(35, 0, 0, "Zabc", 4, 4);
    key(37, 0, 0, "Zabc", 3, 3);
    key(46, 0, 0, "Zab", 3, 3);
    key(8, 0, 0, "Za", 2, 2);
    fields->set(state, "one ++ Я😀", 0);
    key(37, 1, 0, "one ++ Я😀", 7, 7);
    key(37, 1, 1, "one ++ Я😀", 4, 7);
    key(8, 1, 0, "one Я😀", 4, 4);
    key(46, 1, 0, "one ", 4, 4);
    key(65, 1, 0, "one ", 4, 0);
    key(8, 0, 0, "", 0, 0);
    before := state.stamp;
    key(8, 0, 0, "", 0, 0);
    key(46, 0, 0, "", 0, 0);
    if (state.stamp != before) {
        raise "fail: field boundary no-op changed generation";
    }
    fields->set(state, "Я😀abc", 0);
    fields->select(state, 5, 1);
    if (fields->insert(state, "Z") != fields->Ok || state.text != "ЯZ" || state.caret != 2 || state.anchor != 2) {
        raise "fail: field reversed replacement";
    }
    bad := array[] of {0, 9, 10, 11, 12, 13, 16r85, 16r2028, 16r2029};
    before = state.stamp;
    for (index := 0; index < len bad; index++) {
        value := "";
        value[0] = bad[index];
        if (fields->insert(state, value) != fields->Invalid || state.text != "ЯZ" || state.stamp != before) {
            raise "fail: field admitted a control/line separator or changed rejected state";
        }
    }
    text := "x";
    while (len text < fields->MaxCharacters) {
        text += text;
    }
    fields->set(state, text, 0);
    before = state.stamp;
    if (fields->insert(state, "y") != fields->TooLarge || state.text != text || state.stamp != before) {
        raise "fail: field overflow changed state";
    }
    key(65, 1, 0, text, 256, 0);
    if (fields->insert(state, "😀") != fields->Ok || state.text != "😀" || state.caret != 1) {
        raise "fail: field maximum selection did not accept bounded Unicode replacement";
    }
    before = state.stamp;
    if (fields->select(state, -1, 0) != fields->RangeError || state.stamp != before) {
        raise "fail: field invalid selection changed generation";
    }
    state.stamp = fields->MaxStamp;
    if (fields->insert(state, "x") != fields->Invalid || state.text != "😀") {
        raise "fail: field generation wrapped";
    }
    sys->print("PASS Dis field: Unicode selection, words, insertion/deletion, generation and rejected single-line values\n");
}
