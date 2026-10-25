implement Idefield;
include "uibridge.m";
include "idecore.m";
include "ideinput.m";
include "idefield.m";


# Allocate one empty field; ownership remains with the caller's controller process.
create(): ref State
{
    return ref State("", 0, 0, 0);
}


# Validate a complete proposed single-line scalar value before any mutation or slicing.
# text is borrowed; returns Ok, TooLarge or Invalid, with no I/O or native access.
textstatus(text: string): int
{
    if (len text > MaxCharacters) {
        return TooLarge;
    }
    for (index := 0; index < len text; index++) {
        character := text[index];
        if (character <= 0 || character > 16r10ffff || (character >= 16rd800 && character <= 16rdfff)
            || character == '\t' || character == '\n' || character == '\r' || character == 11 || character == 12
            || character == 16r85 || character == 16r2028 || character == 16r2029) {
            return Invalid;
        }
    }
    return Ok;
}


# Reject forged state before indexing its immutable value; state is borrowed.
# Returns a copied status, never repairs invalid coordinates silently.
statevalid(state: ref State): int
{
    if (state == nil || state.caret < 0 || state.anchor < 0 || state.caret > len state.text
        || state.anchor > len state.text || state.stamp < 0 || state.stamp > MaxStamp) {
        return RangeError;
    }
    return textstatus(state.text);
}


# Commit an already constructed complete value and bounded selection as one local generation.
# state is exclusive; all other arguments are values. Any error preserves the previous state exactly.
publish(state: ref State, text: string, anchor, caret: int): int
{
    if (state == nil || anchor < 0 || caret < 0 || anchor > len text || caret > len text) {
        return RangeError;
    }
    status := textstatus(text);
    if (status != Ok) {
        return status;
    }
    if (state.text == text && state.anchor == anchor && state.caret == caret) {
        return Ok;
    }
    if (state.stamp < 0 || state.stamp >= MaxStamp) {
        return Invalid;
    }
    state.text = text;
    state.anchor = anchor;
    state.caret = caret;
    state.stamp++;
    return Ok;
}


# Replace a complete field after validation; all selects the entire new value when nonzero.
# Values/return semantics follow idefield.m. No document/history ownership is acquired.
set(state: ref State, text: string, all: int): int
{
    anchor := len text;
    if (all) {
        anchor = 0;
    }
    return publish(state, text, anchor, len text);
}


# Set caller-supplied scalar endpoints after checking the complete current field state.
# Returns status; generation advances only for an actual selection change.
select(state: ref State, anchor, caret: int): int
{
    status := statevalid(state);
    if (status != Ok) {
        return status;
    }
    return publish(state, state.text, anchor, caret);
}


# Replace exactly the captured field selection after validating insertion and final scalar length.
# state is exclusive, text is borrowed. No failed prefix or generation change becomes visible.
insert(state: ref State, text: string): int
{
    status := statevalid(state);
    if (status != Ok) {
        return status;
    }
    status = textstatus(text);
    if (status != Ok) {
        return status;
    }
    first := state.caret;
    last := state.anchor;
    if (first > last) {
        (first, last) = (last, first);
    }
    if (len state.text - (last - first) + len text > MaxCharacters) {
        return TooLarge;
    }
    result := state.text[:first] + text + state.text[last:];
    return publish(state, result, first + len text, first + len text);
}


# Interpret only field editing operations; callers route Enter/Escape and higher-level commands.
# policy is borrowed for shared word boundaries; other ownership/results follow idefield.m.
apply(policy: Ideinput, state: ref State, event: ref Uibridge->Message, control, shift: int): (int, int)
{
    status := statevalid(state);
    if (status != Ok) {
        return (status, 0);
    }
    if (event == nil || policy == nil) {
        return (RangeError, 0);
    }
    if (event.kind == Uibridge->Key && (event.code < 1 || event.code > 255 || event.flags < 0 || event.flags > 1)) {
        return (Invalid, 0);
    }
    if (event.kind == Uibridge->Text) {
        return (insert(state, event.text), 1);
    }
    if (event.kind != Uibridge->Key || !event.flags) {
        return (Ok, 0);
    }
    code := event.code;
    first := state.caret;
    last := state.anchor;
    if (first > last) {
        (first, last) = (last, first);
    }
    if (control && code == 65) {
        return (select(state, 0, len state.text), 1);
    }
    if (code == 37 || code == 39 || code == 36 || code == 35) {
        position := state.caret;
        if (code == 36) {
            position = 0;
        } else if (code == 35) {
            position = len state.text;
        } else if (!shift && first != last) {
            position = first;
            if (code == 39) {
                position = last;
            }
        } else if (control) {
            position = policy->wordboundary(state.text, position, code == 39);
        } else if (code == 37 && position > 0) {
            position--;
        } else if (code == 39 && position < len state.text) {
            position++;
        }
        anchor := position;
        if (shift) {
            anchor = state.anchor;
        }
        return (select(state, anchor, position), 1);
    }
    if (code == 8 || code == 46) {
        if (first == last && code == 8 && first > 0) {
            first--;
            if (control) {
                first = policy->wordboundary(state.text, state.caret, 0);
            }
        } else if (first == last && code == 46 && last < len state.text) {
            last++;
            if (control) {
                last = policy->wordboundary(state.text, state.caret, 1);
            }
        }
        result := state.text[:first] + state.text[last:];
        return (publish(state, result, first, first), 1);
    }
    return (Ok, 0);
}
