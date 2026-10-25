implement FieldDemo;
include "sys.m"; # Ordinary embedded runtime output; no graphical host is loaded.
include "uibridge.m"; # Field policy accepts copied typed UI values, not native widget references.
include "idecore.m"; # Ideinput's type dependency only; this example never starts a document owner.
include "ideinput.m"; # Shared word-boundary/navigation policy, also used by source editing.
include "idefield.m"; # Reusable, exclusively owned single-line Unicode state.

FieldDemo: module
{
    init: fn(); # Run a deterministic Unicode replacement and rejected multiline insertion.
};


# Demonstrate field ownership, scalar selection and atomic rejection without any document or native UI.
# No parameters; loaded modules and the exclusive field root live until this entry returns.
# Missing modules or mismatched results raise a visible failure instead of printing success.
init()
{
    sys := load Sys Sys->PATH; # Only output crosses into the host service.
    fields := load Idefield Idefield->PATH; # All text/selection mutation remains in actual Dis code.
    if (sys == nil || fields == nil) {
        raise "fail: field tutorial modules";
    }
    state := fields->create(); # [GC:] This process exclusively owns the mutable state and its text root.
    if (fields->set(state, "Я😀abc", 0) != fields->Ok) {
        raise "fail: field tutorial initial value";
    }
    # Reverse selection runs from scalar five back to one, selecting emoji and all ASCII suffix letters.
    # Byte offsets would split Unicode here; both public endpoints deliberately count scalars.
    if (fields->select(state, 5, 1) != fields->Ok || fields->insert(state, "X") != fields->Ok
        || state.text != "ЯX" || state.caret != 2 || state.anchor != 2) {
        raise "fail: field tutorial reverse Unicode replacement";
    }
    generation := state.stamp; # Preserve the accepted generation for an independent rejection check.
    # Validation sees the entire insertion before replacing anything. Neither the accepted prefix nor
    # selection/generation may change when a later scalar makes the proposed value multiline.
    if (fields->insert(state, "prefix\nrejected") != fields->Invalid || state.text != "ЯX"
        || state.caret != 2 || state.anchor != 2 || state.stamp != generation) {
        raise "fail: field tutorial rejected prefix leaked";
    }
    sys->print("PASS field tutorial: %s / atomic single-line rejection\n", state.text);
    # [GC:] Returning releases this process's field reference; no channel/native owner retains it.
}
