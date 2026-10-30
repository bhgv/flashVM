implement Asincviews;
include "asincviews.m";


# [GC:] Private immutable cell. Views share it until a writer allocates a replacement.
Cell: adt
{
    value: string; # Immutable scalar payload; object graphs are outside this prototype.
    version: int; # Revision of the original local write.
    source: int; # Stable writer identity; receiving does not alter it.
};


# Private state belongs solely to serve; replies never expose its arrays or cells.
State: adt
{
    revision: int; # Exact command lease, incremented after accepted mutations.
    phase: int; # Enforced BeforeFrame/AfterFrame alternation.
    active: array of int; # Task lifecycle including owner0.
    cells: array of ref Cell; # Flattened task/binding immutable-cell views.
    dirty: array of int; # Independent per-view local-write bits.
    subscribed: array of int; # Remote publication subscriptions per view/binding.
    events: array of Event; # Pending owner-queued notifications.
};


# Allocate explicit initial state; do not rely on fresh scalar arena contents.
initial(): ref State
{
    size := (Tasks + 1) * Bindings;
    state := ref State(1, 0, array[Tasks + 1] of int,
                       array[size] of ref Cell, array[size] of int,
                       array[size] of int, array[0] of Event);
    for (task := 0; task <= Tasks; task++) {
        state.active[task] = 0;
    }
    state.active[0] = 1;
    for (index := 0; index < size; index++) {
        state.cells[index] = nil;
        state.dirty[index] = 0;
        state.subscribed[index] = 0;
    }
    for (binding := 0; binding < Bindings; binding++) {
        state.cells[binding] = ref Cell("", 1, 0);
    }
    return state;
}


# Produce detached public snapshots; immutable strings can safely share storage.
snapshot(state: ref State, id, status: int, events: array of Event): Reply
{
    size := len state.cells;
    values := array[size] of string;
    versions := array[size] of int;
    sources := array[size] of int;
    dirty := array[size] of int;
    active := array[Tasks + 1] of int;
    active[0:] = state.active;
    for (index := 0; index < size; index++) {
        values[index] = "";
        versions[index] = 0;
        sources[index] = 0;
        dirty[index] = state.dirty[index];
        cell := state.cells[index];
        if (cell != nil) {
            values[index] = cell.value;
            versions[index] = cell.version;
            sources[index] = cell.source;
        }
    }
    return Reply(id, state.revision, status, state.phase, active,
                 values, versions, sources, dirty, events);
}


# Return one event only when receiving changes the scalar value from another writer.
# Versions still propagate for equal values, but equal values create no notification.
notification(state: ref State, index: int, cell: ref Cell): int
{
    return state.subscribed[index] && cell.source != index / Bindings &&
           state.cells[index].value != cell.value;
}


# Append a preflight-admitted event and install one immutable received version.
# Receiving never marks a view dirty, so no artificial echo is published later.
receive(state: ref State, index: int, cell: ref Cell)
{
    if (notification(state, index, cell)) {
        events := array[len state.events + 1] of Event;
        events[0:] = state.events;
        events[len state.events] = Event(index / Bindings, index % Bindings,
                                         cell.source, cell.version,
                                         state.cells[index].value, cell.value);
        state.events = events;
    }
    state.cells[index] = cell;
}


# Validate a complete boundary before any publication or dirty-bit consumption.
# Multiple dirty writers to one binding refuse; this chooses no winning writer.
boundary(state: ref State, before: int): int
{
    if (state.phase != 1 - before) {
        return Invalid;
    }
    task: int;
    binding: int;
    selected := array[Bindings] of ref Cell;
    needed := 0;
    for (binding = 0; binding < Bindings; binding++) {
        selected[binding] = nil;
        if (before) {
            for (task = 1; task <= Tasks; task++) {
                index := task * Bindings + binding;
                if (state.active[task] == 1 && state.dirty[index]) {
                    if (selected[binding] != nil) {
                        return Conflict;
                    }
                    selected[binding] = state.cells[index];
                }
            }
            if (selected[binding] != nil) {
                needed += notification(state, binding, selected[binding]);
            }
        } else if (state.dirty[binding]) {
            selected[binding] = state.cells[binding];
        }
        if (!before) {
            for (task = 1; task <= Tasks; task++) {
                if (state.active[task] != 1) {
                    continue;
                }
                index := task * Bindings + binding;
                if (state.dirty[index]) {
                    if (selected[binding] != nil) {
                        return Conflict;
                    }
                } else {
                    needed += notification(state, index, state.cells[binding]);
                }
            }
        }
    }
    if (len state.events + needed > Capacity) {
        return Full;
    }
    for (binding = 0; binding < Bindings; binding++) {
        if (before) {
            if (selected[binding] == nil) {
                continue;
            }
            receive(state, binding, selected[binding]);
            for (task = 1; task <= Tasks; task++) {
                if (state.active[task] == 1) {
                    state.dirty[task * Bindings + binding] = 0;
                }
            }
        } else {
            for (task = 1; task <= Tasks; task++) {
                index := task * Bindings + binding;
                if (state.active[task] == 1 && !state.dirty[index]) {
                    receive(state, index, state.cells[binding]);
                }
            }
            state.dirty[binding] = 0;
        }
    }
    state.phase = before;
    return Ok;
}


# Execute admitted scalar/lifecycle commands. Invalid/conflicting requests preserve state.
# Drain removes only the target's queued events; Retire discards its subscriptions/events.
apply(state: ref State, request: Request): (int, array of Event)
{
    empty := array[0] of Event;
    item: int;
    task := request.task;
    binding := request.binding;
    if (request.operation == BeforeFrame || request.operation == AfterFrame) {
        return (boundary(state, request.operation == BeforeFrame), empty);
    }
    if (task < 0 || task > Tasks) {
        return (Invalid, empty);
    }
    if (request.operation == Spawn) {
        if (task == 0 || state.active[task] != 0 || state.phase != 0) {
            return (Invalid, empty);
        }
        state.active[task] = 1;
        for (item = 0; item < Bindings; item++) {
            state.cells[task * Bindings + item] = state.cells[item];
        }
        return (Ok, empty);
    }
    if (state.active[task] != 1) {
        return (Invalid, empty);
    }
    if (request.operation == Drain || request.operation == Retire) {
        if (request.operation == Retire && task == 0) {
            return (Invalid, empty);
        }
        kept: list of Event;
        drained: list of Event;
        for (at := len state.events - 1; at >= 0; at--) {
            event := state.events[at];
            if (event.subscriber == task) {
                drained = event :: drained;
            } else {
                kept = event :: kept;
            }
        }
        state.events = array[len kept] of Event;
        for (at = 0; kept != nil; kept = tl kept) {
            state.events[at++] = hd kept;
        }
        result := array[len drained] of Event;
        for (at = 0; drained != nil; drained = tl drained) {
            result[at++] = hd drained;
        }
        if (request.operation == Retire) {
            state.active[task] = -1;
            for (item = 0; item < Bindings; item++) {
                index := task * Bindings + item;
                state.cells[index] = nil;
                state.dirty[index] = 0;
                state.subscribed[index] = 0;
            }
            return (Ok, empty);
        }
        return (Ok, result);
    }
    if (binding < 0 || binding >= Bindings) {
        return (Invalid, empty);
    }
    index := task * Bindings + binding;
    if (request.operation == Subscribe && (request.value == "on" || request.value == "off")) {
        state.subscribed[index] = request.value == "on";
        return (Ok, empty);
    }
    if (request.operation == Discard && task != 0) {
        state.cells[index] = state.cells[binding];
        state.dirty[index] = 0;
        return (Ok, empty);
    }
    if (request.operation == Write && len request.value <= 4096 &&
        (task != 0 || state.phase == 1)) {
        if (state.cells[index].value != request.value) {
            state.cells[index] = ref Cell(request.value, state.revision + 1, task);
            state.dirty[index] = 1;
        }
        return (Ok, empty);
    }
    return (Invalid, empty);
}


# Serialize typed requests; phase publication never blocks on a callback consumer.
# [GC:] Only copied reply arrays escape. Shutdown releases all private roots on exit.
serve(requests: chan of Request)
{
    state := initial();
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        status := Invalid;
        events := array[0] of Event;
        stop := 0;
        if (request.id >= 0) {
            if (request.operation == Snapshot) {
                status = Ok;
            } else if (request.revision != state.revision) {
                status = Stale;
            } else if (state.revision == 2147483647) {
                status = Full;
            } else if (request.operation == Shutdown) {
                status = Ok;
                stop = 1;
            } else {
                (status, events) = apply(state, request);
            }
        }
        if (status == Ok && request.operation != Snapshot) {
            state.revision++;
        }
        request.reply <-= snapshot(state, request.id, status, events);
        if (stop) {
            return;
        }
    }
}


# Start a scheduled Dis owner, not an OS-thread AS interpreter. Caller must consume replies.
start(): chan of Request
{
    requests := chan of Request;
    spawn serve(requests);
    return requests;
}
