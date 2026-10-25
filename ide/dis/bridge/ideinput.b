implement Ideinput;
include "uibridge.m";
include "idecore.m";
include "ideinput.m";


# Allocate private editing policy state. No parameters; returns an owned State reference.
create(): ref State
{
    return ref State(0, 0, 0, 0, -1, 24, -1);
}


# Make one synchronous typed document call; ID 1 is private to this sequential reply channel.
# core/document are borrowed, operation/revision/range/text are values. Returns the owner reply.
call(core: Idecore, document: chan of Idecore->Request, operation: int,
     revision: int, first: int, last: int, text: string): Idecore->Reply
{
    response := chan of Idecore->Reply;
    document <-= Idecore->Request(1, operation, revision, first, last, text, "", response);
    return <-response;
}


# Copy a presentation value without exposing controller or document mutable storage to AS.
# state/reply/event are borrowed values; handled is 0/1. Returns an owned View reference.
view(state: ref State, reply: Idecore->Reply, handled: int,
     event: ref Uibridge->Message): ref View
{
    return ref View(reply, state.caret, state.anchor, handled, event);
}


# Collapse or extend a character selection after a navigation command.
# state is exclusive, position is already bounded by the caller. Returns nothing.
move(state: ref State, position: int)
{
    state.caret = position;
    if (!state.shift) {
        state.anchor = position;
    }
}


# Find the logical LF line containing an admitted Unicode position.
# text/position are values; returns a bounded scalar offset without I/O.
linebegin(text: string, position: int): int
{
    while (position > 0 && text[position - 1] != '\n') {
        position--;
    }
    return position;
}


# Find the logical line end, excluding a CR immediately before LF or EOF.
# text/start are values; returns an insertion coordinate without changing text.
lineend(text: string, start: int): int
{
    last := start;
    while (last < len text && text[last] != '\n') {
        last++;
    }
    if (last > start && text[last - 1] == '\r') {
        last--;
    }
    return last;
}


# Classify one Unicode scalar for deterministic editor word commands, without locale state.
# Returns 0 for spacing, 1 for ASCII identifiers/non-ASCII text, 2 for ASCII punctuation.
# This is a scalar policy, not Unicode UAX #29 segmentation or a grapheme algorithm.
wordclass(character: int): int
{
    if (character == ' ' || character == '\t' || character == '\r' || character == '\n'
        || character == 11 || character == 12 || character == 16r85 || character == 16ra0
        || character == 16r1680 || (character >= 16r2000 && character <= 16r200a)
        || character == 16r2028 || character == 16r2029 || character == 16r202f
        || character == 16r205f || character == 16r3000) {
        return 0;
    }
    if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
        || (character >= '0' && character <= '9') || character == '_' || character >= 128) {
        return 1;
    }
    return 2;
}


# Find the next/previous scalar word boundary in an acknowledged source snapshot.
# position is bounded; forward is 0/1. Returns an insertion coordinate, without I/O or mutation.
wordboundary(text: string, position, forward: int): int
{
    if (position < 0) {
        position = 0;
    }
    if (position > len text) {
        position = len text;
    }
    if (forward) {
        if (position < len text) {
            category := wordclass(text[position]);
            while (position < len text && wordclass(text[position]) == category) {
                position++;
            }
            if (category == 0) {
                return position;
            }
        }
        while (position < len text && wordclass(text[position]) == 0) {
            position++;
        }
    } else {
        while (position > 0 && wordclass(text[position - 1]) == 0) {
            position--;
        }
        if (position > 0) {
            category := wordclass(text[position - 1]);
            while (position > 0 && wordclass(text[position - 1]) == category) {
                position--;
            }
        }
    }
    return position;
}


# Move through logical lines while retaining the requested Unicode column.
# state is exclusive; text is the acknowledged snapshot, delta is bounded +/-128.
# Returns nothing; short lines clamp only this movement, never the preferred column.
vertical(state: ref State, text: string, delta: int)
{
    start := linebegin(text, state.caret);
    if (state.preferred < 0) {
        state.preferred = state.caret - start;
    }
    while (delta > 0) {
        next := start;
        while (next < len text && text[next] != '\n') {
            next++;
        }
        if (next == len text) {
            break;
        }
        start = next + 1;
        delta--;
    }
    while (delta < 0 && start > 0) {
        start = linebegin(text, start - 1);
        delta++;
    }
    last := lineend(text, start);
    position := start + state.preferred;
    if (position > last) {
        position = last;
    }
    move(state, position);
}


# Resolve a source location using the same logical-line rules as keyboard navigation.
# text is borrowed, line/column are positive one-based values. Returns -1 for invalid/absent lines.
locate(text: string, line, column: int): int
{
    if (line < 1 || column < 1) {
        return -1;
    }
    start := 0;
    while (line > 1) {
        while (start < len text && text[start] != '\n') {
            start++;
        }
        if (start == len text) {
            return -1;
        }
        start++;
        line--;
    }
    last := lineend(text, start);
    if (column - 1 > last - start) {
        return last;
    }
    return start + column - 1;
}


# Join private line fragments in logarithmic recursion depth, avoiding quadratic append work.
# pieces is borrowed; low/high bound its half-open interval. Returns a new immutable string.
joinlines(pieces: array of string, low, high: int): string
{
    if (high - low == 1) {
        return pieces[low];
    }
    middle := low + (high - low) / 2;
    return joinlines(pieces, low, middle) + joinlines(pieces, middle, high);
}


# Indent selected logical lines or unindent the caret line through one owner Replace.
# state is exclusive, reply is acknowledged, remove is 0/1. Returns the real owner result.
# Selected ranges expand to whole affected lines while retaining selection direction.
indent(core: Idecore, document: chan of Idecore->Request, state: ref State,
       reply: Idecore->Reply, remove: int): Idecore->Reply
{
    first := state.caret;
    last := state.anchor;
    if (first > last) {
        (first, last) = (last, first);
    }
    selected := first != last;
    first = linebegin(reply.text, first);
    if (selected && last > first && reply.text[last - 1] == '\n') {
        last--; # An endpoint at the following line start excludes that line.
    }
    while (last < len reply.text && reply.text[last] != '\n') {
        last++;
    }
    if (last < len reply.text) {
        last++;
    }
    lines := 1;
    for (position := first; position < last - 1; position++) {
        if (reply.text[position] == '\n') {
            lines++;
        }
    }
    capacity := last - first;
    if (!remove) {
        capacity += lines * 4;
    }
    if (capacity > core->MaxCharacters || len reply.text - (last - first) + capacity > core->MaxCharacters) {
        reply.status = core->TooLarge;
        return reply;
    }
    pieces := array[lines] of string; # Bounded per-line fragments are joined as a balanced tree.
    used := 0;
    position = first;
    removedfirst := 0;
    for (;;) {
        begin := position;
        removed := 0;
        if (remove) {
            if (position < last && reply.text[position] == '\t') {
                position++;
                removed = 1;
            } else {
                while (position < last && removed < 4 && reply.text[position] == ' ') {
                    position++;
                    removed++;
                }
            }
        }
        if (begin == first) {
            removedfirst = removed;
        }
        begin = position;
        while (position < last) {
            character := reply.text[position++];
            if (character == '\n') {
                break;
            }
        }
        piece := reply.text[begin:position];
        if (!remove) {
            piece = "    " + piece;
        }
        pieces[used++] = piece;
        if (position >= last) {
            break;
        }
    }
    insertion := joinlines(pieces, 0, used);
    used = len insertion;
    if (insertion == reply.text[first:last]) {
        return reply; # A no-op unindent must not consume a revision or Undo entry.
    }
    result := call(core, document, core->Replace, state.revision, first, last, insertion);
    if (result.status == core->Ok) {
        if (selected) {
            if (state.caret < state.anchor) {
                state.caret = first;
                state.anchor = first + used;
            } else {
                state.anchor = first;
                state.caret = first + used;
            }
        } else {
            state.caret -= removedfirst;
            if (state.caret < first) {
                state.caret = first;
            }
            state.anchor = state.caret;
        }
        state.preferred = -1;
    }
    state.revision = result.revision;
    return result;
}


# Apply one input against a fresh document snapshot, preserving revision conflicts visibly.
# Arguments/results follow ideinput.m; all edit/history policy stays in this Dis module.
apply(core: Idecore, document: chan of Idecore->Request,
      state: ref State, event: ref Uibridge->Message): (int, ref View)
{
    if (core == nil || document == nil || state == nil || event == nil
        || event.kind < Uibridge->Pointer || event.kind > Uibridge->Focus) {
        return (0, nil);
    }
    if (event.kind == Uibridge->Key && (event.code < 1 || event.code > 255
        || event.flags < 0 || event.flags > 1)) {
        return (0, nil);
    }
    if (event.kind == Uibridge->Focus) {
        if (event.flags < 0 || event.flags > 1) {
            return (0, nil);
        }
        if (!event.flags) {
            state.control = 0;
            state.shift = 0;
        }
    }
    if (event.kind == Uibridge->Key && event.code == 17) {
        state.control = event.flags;
    }
    if (event.kind == Uibridge->Key && event.code == 16) {
        state.shift = event.flags;
    }
    reply := call(core, document, core->Snapshot, 0, 0, 0, "");
    if (state.revision >= 0 && state.revision != reply.revision) {
        state.revision = reply.revision;
        state.caret = 0;
        state.anchor = 0;
        state.preferred = -1;
        return (1, view(state, reply, 0, event));
    }
    state.revision = reply.revision;
    if (state.caret < 0 || state.caret > len reply.text) {
        state.caret = len reply.text;
    }
    if (state.anchor < 0 || state.anchor > len reply.text) {
        state.anchor = state.caret;
    }
    if (event.kind == Uibridge->Focus) {
        return (1, view(state, reply, 1, event));
    }
    if (!reply.opened) {
        return (1, view(state, reply, 0, event));
    }
    if (event.kind == Uibridge->Key && (!event.flags || event.code == 16 || event.code == 17)) {
        return (1, view(state, reply, 1, event));
    }
    first := state.caret;
    last := state.anchor;
    if (first > last) {
        (first, last) = (last, first);
    }
    operation := -1;
    insertion := "";
    destination := state.caret;
    if (event.kind == Uibridge->Text) {
        operation = core->Replace;
        insertion = event.text;
        destination = first + len insertion;
    } else if (event.kind == Uibridge->Key) {
        key := event.code;
        if (state.control && key == 65) {
            state.anchor = 0;
            state.caret = len reply.text;
            state.preferred = -1;
            return (1, view(state, reply, 1, event));
        }
        if (!state.control && key == 9) {
            if (first != last || state.shift) {
                result := indent(core, document, state, reply, state.shift);
                return (1, view(state, result, 1, event));
            }
            spaces := 4 - (first - linebegin(reply.text, first)) % 4;
            insertion = "    "[:spaces];
            operation = core->Replace;
            destination = first + spaces;
        } else if (!state.control && key == 13) {
            operation = core->Replace;
            insertion = "\n";
            destination = first + 1;
        } else if (state.control && key == 90) {
            operation = core->Undo;
            if (state.shift) {
                operation = core->Redo;
            }
        } else if (state.control && key == 89) {
            operation = core->Redo;
        } else if (key == 37 || key == 39) {
            position := state.caret;
            if (!state.shift && first != last) {
                position = first;
                if (key == 39) {
                    position = last;
                }
            } else if (state.control) {
                position = wordboundary(reply.text, position, key == 39);
            } else if (key == 37 && position > 0) {
                position--;
                if (position > 0 && reply.text[position] == '\n' && reply.text[position - 1] == '\r') {
                    position--;
                }
            } else if (key == 39 && position < len reply.text) {
                position++;
                if (position < len reply.text && reply.text[position - 1] == '\r' && reply.text[position] == '\n') {
                    position++;
                }
            }
            state.preferred = -1;
            move(state, position);
            return (1, view(state, reply, 1, event));
        } else if (!state.control && (key == 38 || key == 40 || key == 33 || key == 34)) {
            delta := 1;
            if (key == 33 || key == 34) {
                delta = state.page;
                if (delta < 1 || delta > 128) {
                    delta = 24;
                }
            }
            if (key == 38 || key == 33) {
                delta = -delta;
            }
            vertical(state, reply.text, delta);
            return (1, view(state, reply, 1, event));
        } else if (key == 36 || key == 35) {
            position := 0;
            if (state.control) {
                if (key == 35) {
                    position = len reply.text;
                }
            } else if (key == 36) {
                position = linebegin(reply.text, state.caret);
            } else {
                position = lineend(reply.text, linebegin(reply.text, state.caret));
            }
            state.preferred = -1;
            move(state, position);
            return (1, view(state, reply, 1, event));
        } else if (key == 8 || key == 46) {
            if (first == last && key == 8 && first > 0) {
                if (state.control) {
                    first = wordboundary(reply.text, first, 0);
                } else {
                    first--;
                    if (first > 0 && reply.text[first] == '\n' && reply.text[first - 1] == '\r') {
                        first--;
                    }
                }
            } else if (first == last && key == 46 && last < len reply.text) {
                if (state.control) {
                    last = wordboundary(reply.text, last, 1);
                } else {
                    last++;
                    if (last < len reply.text && reply.text[last - 1] == '\r' && reply.text[last] == '\n') {
                        last++;
                    }
                }
            }
            if (first == last) {
                return (1, view(state, reply, 1, event));
            }
            operation = core->Replace;
            destination = first;
        }
    }
    if (operation < 0) {
        return (1, view(state, reply, 0, event));
    }
    reply = call(core, document, operation, state.revision, first, last, insertion);
    if (reply.status == core->Ok) {
        if (destination > len reply.text) {
            destination = len reply.text;
        }
        state.caret = destination;
        state.anchor = destination;
        state.preferred = -1;
    }
    state.revision = reply.revision;
    return (1, view(state, reply, 1, event));
}


# Change selection only against the controller's current acknowledged revision.
# state/reply/range ownership and return/error semantics follow ideinput.m.
select(state: ref State, reply: Idecore->Reply, first, last: int): int
{
    if (state == nil || !reply.opened || state.revision != reply.revision
        || first < 0 || last < 0 || first > len reply.text || last > len reply.text) {
        return 0;
    }
    state.anchor = first;
    state.caret = last;
    state.preferred = -1;
    return 1;
}


# Synchronize private selection after an explicitly acknowledged external document command.
# state is exclusive, reply is a value snapshot. Returns nothing and performs no I/O.
observe(state: ref State, reply: Idecore->Reply)
{
    if (state.revision != reply.revision || !reply.opened) {
        state.caret = 0;
        state.anchor = 0;
        state.preferred = -1;
    }
    state.revision = reply.revision;
}


# Own one controller event loop while borrowing the document's separate owner process.
# Parameters/stop semantics follow ideinput.m; each admitted event publishes one View.
serve(core: Idecore, document: chan of Idecore->Request,
      input: chan of ref Uibridge->Message, views: chan of ref View): int
{
    if (core == nil || document == nil || input == nil || views == nil) {
        return -1;
    }
    state := create();
    for (;;) {
        event := <-input;
        if (event == nil) {
            return 0;
        }
        (ok, result) := apply(core, document, state, event);
        if (!ok) {
            return -1;
        }
        views <-= result;
    }
}
