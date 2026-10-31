implement TaskTest;
include "sys.m";
include "asinctasks.m";
TaskTest: module { init: fn(); }; # Executable owner lifecycle regression.
tasks: Asinctasks; # Loaded bytecode rooted throughout this test.
owner: chan of Asinctasks->Request; # Sole service owner under test.
checks: int; # Active predicates, including optimized builds.


# Fail explicitly on a false predicate, with context; no disabled assertion oracle.
check(condition: int, message: string)
{
    if (!condition) {
        raise "fail: asinc task " + message;
    }
    checks++;
}


# Send and consume a typed request using supplied generation/task/worker/lease/value.
# Blocks only on the owner and its reply. No render or timeline dependency exists.
call(generation, operation, task, worker, lease: int, value: string): Asinctasks->Reply
{
    replies := chan of Asinctasks->Reply;
    owner <-= Asinctasks->Request(7, generation, operation, task, worker, lease, value, replies);
    reply := <-replies;
    check(reply.id == 7, "reply correlation");
    return reply;
}


# Consume an expected successful operation in current incarnation17; return its reply.
accept(operation, task, worker, lease: int, value: string): Asinctasks->Reply
{
    reply := call(17, operation, task, worker, lease, value);
    check(reply.status == tasks->Ok, "accepted operation " + string operation);
    return reply;
}


# A real scheduled worker holds its own scalar input until commanded to finish.
# ready transfers claim details; go controls order without sleeps or timing assumptions.
# This process consumes every owner acknowledgement before reporting done.
worker(ready: chan of Asinctasks->Reply, go: chan of int,
       done: chan of Asinctasks->Reply)
{
    claim := call(17, tasks->Claim, 0, 41, 0, "");
    ready <-= claim;
    <-go;
    done <-= call(17, tasks->Complete, claim.task, 41, claim.lease, claim.value + " result");
}


# Force both race orders against an actual worker, including independently retained input.
lifecycle()
{
    first := accept(tasks->Submit, 0, 0, 0, "capture");
    ready := chan of Asinctasks->Reply;
    go := chan of int;
    done := chan of Asinctasks->Reply;
    spawn worker(ready, go, done);
    claim := <-ready;
    check(claim.status == tasks->Ok && claim.task == first.task && claim.value == "capture",
          "worker receives scalar environment");
    check(call(17, tasks->Shutdown, 0, 0, 0, "").status == tasks->Busy,
          "running worker prevents shutdown");
    check(call(17, tasks->Complete, claim.task, 42, claim.lease, "wrong").status == tasks->Stale &&
          call(17, tasks->Complete, claim.task, 41, claim.lease + 1, "wrong").status == tasks->Stale,
          "worker identity and lease checked independently");
    accept(tasks->Cancel, claim.task, 0, 0, "");
    snapshot := accept(tasks->Snapshot, 0, 0, 0, "");
    check(snapshot.tasks[0].state == tasks->CancelRequested && snapshot.tasks[0].inputHeld == 1,
          "cancel request retains input until worker acknowledgement");
    check(call(17, tasks->Shutdown, 0, 0, 0, "").status == tasks->Busy,
          "cancelling worker prevents shutdown");
    go <-= 1;
    finish := <-done;
    check(finish.status == tasks->Ok, "late completion acknowledges cancellation");
    snapshot = accept(tasks->Snapshot, 0, 0, 0, "");
    check(snapshot.tasks[0].state == tasks->Cancelled && snapshot.tasks[0].inputHeld == 0 &&
          snapshot.tasks[0].resultHeld == 0 && snapshot.tasks[0].input == nil &&
          snapshot.tasks[0].result == nil, "cancel winner releases input and discards result");
    check(call(17, tasks->Complete, claim.task, 41, claim.lease, "duplicate").status == tasks->Terminal,
          "duplicate finish cannot revive cancelled task");
    second := accept(tasks->Submit, 0, 0, 0, "next");
    spawn worker(ready, go, done);
    claim = <-ready;
    go <-= 1;
    finish = <-done;
    check(finish.status == tasks->Ok && claim.task == second.task, "completion wins first");
    check(call(17, tasks->Cancel, claim.task, 0, 0, "").status == tasks->Terminal,
          "later cancellation cannot overwrite completion");
    snapshot = accept(tasks->Snapshot, 0, 0, 0, "");
    check(snapshot.tasks[1].state == tasks->Completed && snapshot.tasks[1].inputHeld == 0 &&
          snapshot.tasks[1].resultHeld == 1, "result lifetime independent from input");
    snapshot.tasks[1].result[0] = 'X';
    check(accept(tasks->Snapshot, 0, 0, 0, "").tasks[1].result == "next result",
          "scalar string write detaches from owner value");
    snapshot.tasks[1].result = "external mutation";
    snapshot.tasks[1].state = tasks->Cancelled;
    result := accept(tasks->TakeResult, second.task, 0, 0, "");
    check(result.value == "next result", "detached snapshot cannot change retained result");
    check(call(17, tasks->TakeResult, second.task, 0, 0, "").status == tasks->Taken,
          "result transferred exactly once");
    snapshot = accept(tasks->Snapshot, 0, 0, 0, "");
    check(snapshot.tasks[1].resultHeld == 0 && snapshot.tasks[1].result == nil,
          "take clears owner result root");
}


# Retire the full table, then reuse slots without reusing identities or FIFO position.
# Runs under the same owner incarnation; old worker replies must never name new work.
recycling()
{
    full := accept(tasks->Snapshot, 0, 0, 0, "");
    check(len full.tasks == tasks->Capacity, "start recycling at retained capacity");
    for (index := 0; index < len full.tasks; index++) {
        task := full.tasks[index];
        if (task.state == tasks->Queued) {
            accept(tasks->Cancel, task.id, 0, 0, "");
        }
        if (task.resultHeld) {
            accept(tasks->TakeResult, task.id, 0, 0, "");
        }
        accept(tasks->Retire, task.id, 0, 0, "");
    }
    check(len accept(tasks->Snapshot, 0, 0, 0, "").tasks == 0,
          "all terminal records removed");
    check(call(17, tasks->Complete, 1, 41, 1, "late").status == tasks->Stale &&
          call(17, tasks->Cancel, 1, 0, 0, "").status == tasks->Stale &&
          call(17, tasks->TakeResult, 2, 0, 0, "").status == tasks->Stale,
          "retired IDs refuse late finish/cancel/take");
    check(call(17, tasks->Retire, 0, 0, 0, "").status == tasks->Invalid &&
          call(17, tasks->Retire, 2147483647, 0, 0, "").status == tasks->Invalid,
          "zero and never-issued IDs remain invalid");
    first := accept(tasks->Submit, 0, 0, 0, "first");
    older := accept(tasks->Submit, 0, 0, 0, "older");
    check(first.task == tasks->Capacity + 1 && older.task == first.task + 1,
          "new IDs advance beyond former lifetime limit");
    check(call(17, tasks->Retire, first.task, 0, 0, "").status == tasks->Busy,
          "queued record cannot be retired");
    claim := accept(tasks->Claim, 0, 77, 0, "");
    check(claim.task == first.task &&
          call(17, tasks->Retire, first.task, 0, 0, "").status == tasks->Busy,
          "claimed record cannot be retired");
    accept(tasks->Cancel, first.task, 0, 0, "");
    check(call(17, tasks->Retire, first.task, 0, 0, "").status == tasks->Busy,
          "cancelling record waits for acknowledgement");
    accept(tasks->AcknowledgeCancel, first.task, 77, claim.lease, "");
    accept(tasks->Retire, first.task, 0, 0, "");
    newer := accept(tasks->Submit, 0, 0, 0, "newer");
    snapshot := accept(tasks->Snapshot, 0, 0, 0, "");
    check(len snapshot.tasks == 2 && snapshot.tasks[0].id == older.task &&
          snapshot.tasks[1].id == newer.task, "snapshot sorted despite recycled slot order");
    claim = accept(tasks->Claim, 0, 77, 0, "");
    check(claim.task == older.task && claim.value == "older",
          "old queued work precedes newer work in a lower storage slot");
    check(call(17, tasks->Complete, first.task, 77, first.task, "late").status == tasks->Stale &&
          call(17, tasks->Complete, older.task, 77, first.task, "wrong lease").status == tasks->Stale,
          "late retired worker and old lease cannot complete replacement");
    accept(tasks->Complete, older.task, 77, claim.lease, "");
    retained := accept(tasks->Snapshot, 0, 0, 0, "");
    check(retained.tasks[0].resultHeld == 1 && len retained.tasks[0].result == 0 &&
          call(17, tasks->Retire, older.task, 0, 0, "").status == tasks->Busy,
          "empty valid result still prevents retirement before take");
    check(accept(tasks->TakeResult, older.task, 0, 0, "").value == "",
          "empty result transfers successfully");
    accept(tasks->Retire, older.task, 0, 0, "");
    claim = accept(tasks->Claim, 0, 78, 0, "");
    check(claim.task == newer.task, "newer queued work remains available");
    accept(tasks->Fail, newer.task, 78, claim.lease, "failure");
    check(call(17, tasks->Retire, newer.task, 0, 0, "").status == tasks->Busy,
          "untaken error result also prevents retirement");
    check(accept(tasks->TakeResult, newer.task, 0, 0, "").value == "failure", "error transfers");
    accept(tasks->Retire, newer.task, 0, 0, "");
    check(snapshot.tasks[0].input == "older" && snapshot.tasks[1].input == "newer",
          "old detached snapshot survives owner retirement and slot reuse");
    previous := newer.task;
    for (round := 0; round < tasks->Capacity * 4; round++) {
        submitted := accept(tasks->Submit, 0, 0, 0, string round);
        check(submitted.task == previous + 1, "identity monotonic across repeated slot reuse");
        claim = accept(tasks->Claim, 0, 87, 0, "");
        check(claim.task == submitted.task && claim.value == string round,
              "recycled slot retains only current input");
        check(call(17, tasks->Complete, previous, 87, previous, "late").status == tasks->Stale,
              "previous identity stays stale while replacement is running");
        accept(tasks->Complete, submitted.task, 87, claim.lease, "value " + string round);
        check(accept(tasks->TakeResult, submitted.task, 0, 0, "").value == "value " + string round,
              "only current result delivered");
        accept(tasks->Retire, submitted.task, 0, 0, "");
        check(call(17, tasks->Retire, submitted.task, 0, 0, "").status == tasks->Stale,
              "duplicate retirement is stale");
        previous = submitted.task;
    }
    check(len accept(tasks->Snapshot, 0, 0, 0, "").tasks == 0, "bounded table empty after churn");
}


# Cover queued cancellation, explicit cancellation acknowledgement, failure and capacity.
# Retained records fill capacity until explicitly retired; IDs themselves never recycle.
boundaries()
{
    queued := accept(tasks->Submit, 0, 0, 0, "queued");
    accept(tasks->Cancel, queued.task, 0, 0, "");
    check(call(17, tasks->Claim, 0, 1, 0, "").status == tasks->Empty,
          "queued cancellation cannot be claimed");
    pending := accept(tasks->Submit, 0, 0, 0, "cancel me");
    claim := accept(tasks->Claim, 0, 8, 0, "");
    check(call(17, tasks->AcknowledgeCancel, pending.task, 8, claim.lease, "").status == tasks->Invalid,
          "unsolicited cancellation acknowledgement refused");
    accept(tasks->Cancel, pending.task, 0, 0, "");
    accept(tasks->AcknowledgeCancel, pending.task, 8, claim.lease, "");
    failed := accept(tasks->Submit, 0, 0, 0, "failure input");
    claim = accept(tasks->Claim, 0, 9, 0, "");
    accept(tasks->Fail, failed.task, 9, claim.lease, "worker error");
    check(accept(tasks->TakeResult, failed.task, 0, 0, "").value == "worker error",
          "failure result transferred");
    check(call(16, tasks->Submit, 0, 0, 0, "stale").status == tasks->Stale &&
          call(18, tasks->Snapshot, 0, 0, 0, "").status == tasks->Stale,
          "old and future owner incarnations rejected");
    huge := "";
    for (index := 0; index < 65537; index++) {
        huge[len huge] = 'x';
    }
    check(call(17, tasks->Submit, 0, 0, 0, huge).status == tasks->Invalid,
          "oversized environment refused before insertion");
    snapshot := accept(tasks->Snapshot, 0, 0, 0, "");
    for (index = len snapshot.tasks; index < tasks->Capacity; index++) {
        accepted := accept(tasks->Submit, 0, 0, 0, "");
        check(accepted.task == index + 1, "monotonic nonrecycled ID");
    }
    check(call(17, tasks->Submit, 0, 0, 0, "overflow").status == tasks->Full,
          "retained capacity refuses without silent record removal");
    recycling();
    accept(tasks->Shutdown, 0, 0, 0, "");
    owner = tasks->start(18);
    check(call(17, tasks->Snapshot, 0, 0, 0, "").status == tasks->Stale,
          "new owner rejects previous incarnation");
    check(call(18, tasks->Shutdown, 0, 0, 0, "").status == tasks->Ok,
          "new incarnation can shut down");
}


# Load ordinary portable bytecode and exercise actual scheduled Dis channel ownership.
init()
{
    sys := load Sys Sys->PATH;
    tasks = load Asinctasks Asinctasks->PATH;
    check(tasks != nil, "module load");
    check(tasks->start(0) == nil, "invalid generation returns nil");
    owner = tasks->start(17);
    lifecycle();
    boundaries();
    sys->print("PASS asinc task lifecycle: %d checks, completion/cancel order, claim leases, root release and incarnations\n", checks);
}
