implement Asinctasks;
include "asinctasks.m";


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


# Apply one incarnation-checked request to sole-owner tasks; return an acknowledgement.
# No callbacks or frame waits occur. Allocation failures propagate; no OOM recovery claim.
# The sole receive order determines whether completion or cancellation wins.
apply(tasks: array of Task, count: int, request: Request): Reply
{
    reply := Reply(request.id, Invalid, request.task, 0, nil, nil);
    if (request.operation == Snapshot) {
        reply.tasks = array[count] of Task;
        reply.tasks[0:] = tasks[0:count];
        reply.status = Ok;
        return reply;
    }
    if (request.operation == Submit) {
        if (count == Capacity) {
            reply.status = Full;
        } else if (len request.value <= 65536) {
            tasks[count] = Task(count + 1, Queued, 0, 0, request.value, nil, 1, 0);
            reply.task = count + 1;
            reply.status = Ok;
        }
        return reply;
    }
    if (request.operation == Claim) {
        if (request.worker <= 0) {
            return reply;
        }
        reply.status = Empty;
        for (index := 0; index < count; index++) {
            if (tasks[index].state == Queued) {
                tasks[index].state = Running;
                tasks[index].worker = request.worker;
                tasks[index].lease = tasks[index].id;
                reply.task = tasks[index].id;
                reply.lease = tasks[index].lease;
                reply.value = tasks[index].input;
                reply.status = Ok;
                break;
            }
        }
        return reply;
    }
    if (request.operation == Shutdown) {
        reply.status = Ok;
        for (index := 0; index < count; index++) {
            if (tasks[index].state == Running || tasks[index].state == CancelRequested) {
                reply.status = Busy;
                return reply;
            }
        }
        # [GC:] Acknowledged shutdown discards queued inputs and untaken results.
        for (index = 0; index < count; index++) {
            releaseinput(tasks, index);
            tasks[index].result = nil;
            tasks[index].resultHeld = 0;
        }
        return reply;
    }
    if (request.task <= 0 || request.task > count) {
        return reply;
    }
    index := request.task - 1;
    state := tasks[index].state;
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
# No task IDs or claim tokens are recycled; a new owner needs a new caller generation.
serve(requests: chan of Request, incarnation: int)
{
    tasks := array[Capacity] of Task;
    count := 0;
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
                reply = apply(tasks, count, request);
            }
        }
        if (reply.status == Ok && request.operation == Submit) {
            count++;
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
