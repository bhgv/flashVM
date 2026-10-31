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


# Cover queued cancellation, explicit cancellation acknowledgement, failure and capacity.
# IDs remain occupied after terminal release, so no late message names a different task.
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
          "lifetime capacity refuses without reuse");
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
