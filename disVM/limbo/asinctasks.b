implement Asinctasks;
include "asinctasks.m";


# [GC:] Sole owner retains only this bounded table; zero ID denotes a free record.
State: adt
{
    tasks: array of Task; # Fixed-capacity storage, never exposed by reference.
    count: int;           # Number of positive-ID records currently retained.
    lastid: int;          # Last issued identity; monotonic and never allowed to wrap.
};


# Borrow state and a positive identity; return its slot, or -1 for a retired ID.
# No allocation or blocking occurs; caller checks identity against lastid first.
findtask(state: ref State, id: int): int
{
    for (index := 0; index < Capacity; index++) {
        if (state.tasks[index].id == id) {
            return index;
        }
    }
    return -1;
}


# Copy live records sorted by creation ID, independent of storage-slot recycling.
# Borrow owner state; return detached records with scalar value/COW semantics.
# Allocation errors propagate; neither state mutation nor channel blocking occurs.
observe(state: ref State): array of Task
{
    result := array[state.count] of Task;
    count := 0;
    for (slot := 0; slot < Capacity; slot++) {
        if (state.tasks[slot].id > 0) {
            task := state.tasks[slot];
            index := count;
            while (index > 0 && result[index - 1].id > task.id) {
                result[index] = result[index - 1];
                index--;
            }
            result[index] = task;
            count++;
        }
    }
    return result;
}


# Inspect one detached task state; return whether no worker can still finish it.
terminal(state: int): int
{
    return state == Completed || state == Failed || state == Cancelled;
}


# [GC:] Release only this owner's input root after worker acknowledgement/terminal entry.
# Borrow tasks and valid index. Immutable copies already held by workers remain theirs.
releaseinput(tasks: array of Task, index: int)
{
    tasks[index].input = nil;
    tasks[index].inputHeld = 0;
}


# Apply an incarnation-checked request to private owner state; return an acknowledgement.
# No callbacks or frame waits occur. Allocation failures propagate; no OOM recovery claim.
# The sole receive order determines whether completion or cancellation wins.
apply(owner: ref State, request: Request): Reply
{
    tasks := owner.tasks;
    reply := Reply(request.id, Invalid, request.task, 0, nil, nil);
    if (request.operation == Snapshot) {
        reply.tasks = observe(owner);
        reply.status = Ok;
        return reply;
    }
    if (request.operation == Submit) {
        if (owner.count == Capacity || owner.lastid == 2147483647) {
            reply.status = Full;
        } else if (len request.value <= 65536) {
            for (slot := 0; slot < Capacity; slot++) {
                if (tasks[slot].id == 0) {
                    owner.lastid++;
                    tasks[slot] = Task(owner.lastid, Queued, 0, 0, request.value, nil, 1, 0);
                    owner.count++;
                    reply.task = owner.lastid;
                    reply.status = Ok;
                    break;
                }
            }
        }
        return reply;
    }
    if (request.operation == Claim) {
        if (request.worker <= 0) {
            return reply;
        }
        # Earliest creation ID wins, even if a newer task reused an earlier slot.
        selected := -1;
        for (index := 0; index < Capacity; index++) {
            if (tasks[index].id > 0 && tasks[index].state == Queued &&
                (selected < 0 || tasks[index].id < tasks[selected].id)) {
                selected = index;
            }
        }
        reply.status = Empty;
        if (selected >= 0) {
            tasks[selected].state = Running;
            tasks[selected].worker = request.worker;
            tasks[selected].lease = tasks[selected].id;
            reply.task = tasks[selected].id;
            reply.lease = tasks[selected].lease;
            reply.value = tasks[selected].input;
            reply.status = Ok;
        }
        return reply;
    }
    if (request.operation == Shutdown) {
        reply.status = Ok;
        for (index := 0; index < Capacity; index++) {
            if (tasks[index].id > 0 &&
                (tasks[index].state == Running || tasks[index].state == CancelRequested)) {
                reply.status = Busy;
                return reply;
            }
        }
        # [GC:] Acknowledged shutdown discards queued inputs and untaken results.
        for (index = 0; index < Capacity; index++) {
            releaseinput(tasks, index);
            tasks[index].result = nil;
            tasks[index].resultHeld = 0;
        }
        return reply;
    }
    if (request.task <= 0 || request.task > owner.lastid) {
        return reply;
    }
    index := findtask(owner, request.task);
    if (index < 0) {
        reply.status = Stale;
        return reply;
    }
    state := tasks[index].state;
    if (request.operation == Retire) {
        if (!terminal(state) || tasks[index].resultHeld) {
            reply.status = Busy;
            return reply;
        }
        # [GC:] No worker/result remains. Clear metadata and all payload roots together.
        tasks[index] = Task(0, Queued, 0, 0, nil, nil, 0, 0);
        owner.count--;
        reply.status = Ok;
        return reply;
    }
    if (request.operation == Cancel) {
        if (terminal(state)) {
            reply.status = Terminal;
            return reply;
        }
        if (state == Queued) {
            tasks[index].state = Cancelled;
            releaseinput(tasks, index);
        } else {
            tasks[index].state = CancelRequested;
        }
        reply.status = Ok;
        return reply;
    }
    if (request.operation == TakeResult) {
        if (state == Completed || state == Failed) {
            if (!tasks[index].resultHeld) {
                reply.status = Taken;
                return reply;
            }
            # [GC:] Reply roots the value before the owner's result root is cleared.
            reply.value = tasks[index].result;
            tasks[index].result = nil;
            tasks[index].resultHeld = 0;
            reply.status = Ok;
        } else if (state == Cancelled) {
            reply.status = Terminal;
        } else {
            reply.status = Busy;
        }
        return reply;
    }
    if (request.operation != Complete && request.operation != Fail &&
        request.operation != AcknowledgeCancel) {
        return reply;
    }
    if (request.worker <= 0 || request.worker != tasks[index].worker ||
        request.lease <= 0 || request.lease != tasks[index].lease) {
        reply.status = Stale;
        return reply;
    }
    if (terminal(state)) {
        reply.status = Terminal;
        return reply;
    }
    if (state != Running && state != CancelRequested) {
        return reply;
    }
    if (state == CancelRequested) {
        # Cancellation won before this finish: discard late value; worker is now done.
        tasks[index].state = Cancelled;
        releaseinput(tasks, index);
        reply.status = Ok;
        return reply;
    }
    if (request.operation == AcknowledgeCancel || len request.value > 65536) {
        return reply;
    }
    # [GC:] Retain immutable result before releasing input; delivery is independently owned.
    tasks[index].result = request.value;
    tasks[index].resultHeld = 1;
    tasks[index].state = Completed;
    if (request.operation == Fail) {
        tasks[index].state = Failed;
    }
    releaseinput(tasks, index);
    reply.status = Ok;
    return reply;
}


# Consume typed requests until an accepted Shutdown. Every nonnil reply must be consumed.
# All arrays stay local; snapshots copy records, whose scalar strings have value semantics.
# Storage slots can be retired; task IDs/claim tokens never recycle or wrap.
# A replacement owner still requires a new caller generation.
serve(requests: chan of Request, incarnation: int)
{
    state := ref State(array[Capacity] of Task, 0, 0);
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            request.value = nil;
            continue;
        }
        reply := Reply(request.id, Invalid, request.task, 0, nil, nil);
        if (request.id >= 0) {
            if (request.incarnation != incarnation) {
                reply.status = Stale;
            } else {
                reply = apply(state, request);
            }
        }
        # [GC:] Do not retain transport payloads while blocked for the next request.
        # Slot ownership was already transferred/released by apply; reply owns its copy.
        request.value = nil;
        request.reply <-= reply;
        request.reply = nil;
        reply.value = nil;
        reply.tasks = nil;
        if (reply.status == Ok && request.operation == Shutdown) {
            return;
        }
    }
}


# Spawn a scheduled Dis owner for a caller-chosen positive incarnation; invalid returns nil.
# The caller must never recycle incarnations while stale messages can exist.
# This module does not execute AS closures, create native threads or change Dis64 ABI.
start(incarnation: int): chan of Request
{
    if (incarnation <= 0) {
        return nil;
    }
    requests := chan of Request;
    spawn serve(requests, incarnation);
    return requests;
}
