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
        if (state.control && key == 90) {
            operation = core->Undo;
            if (state.shift) {
                operation = core->Redo;
            }
        } else if (state.control && key == 89) {
            operation = core->Redo;
        } else if (!state.control && (key == 37 || key == 39)) {
            position := state.caret;
            if (!state.shift && first != last) {
                position = first;
                if (key == 39) {
                    position = last;
                }
            } else if (key == 37 && position > 0) {
                position--;
            } else if (key == 39 && position < len reply.text) {
                position++;
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
        } else if (!state.control && (key == 8 || key == 46)) {
            if (first == last && key == 8 && first > 0) {
                first--;
            } else if (first == last && key == 46 && last < len reply.text) {
                last++;
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
