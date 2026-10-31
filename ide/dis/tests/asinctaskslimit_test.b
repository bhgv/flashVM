implement TaskLimitTest;
include "sys.m";
include "asinctasks.m";


# Private near-exhaustion fixture; the host driver supplies its temporary owner.
TaskLimitTest: module
{
    init: fn(); # Run all active predicates and print PASS only on complete success.
};
owner: chan of Asinctasks->Request; # Owner compiled with lastid seeded immediately below signed maximum.
checks: int;                           # Active runtime predicate count.


# Fail on a false condition with a readable message; every predicate executes.
check(condition: int, message: string)
{
    if (!condition) {
        raise "fail: asinc identity limit " + message;
    }
    checks++;
}


# Send an operation in generation1; return the consumed copied acknowledgement.
# Blocks on the sole owner. Payload is deliberately empty, testing held-empty results.
call(operation, task, worker, lease: int): Asinctasks->Reply
{
    replies := chan of Asinctasks->Reply;
    owner <-= Asinctasks->Request(1, 1, operation, task, worker, lease, "", replies);
    return <-replies;
}


# Reach signed identity exhaustion without executing billions of submissions.
# The host driver modifies only the private initial counter in a temporary module.
# It uses the real production admission/claim/finish/retire paths thereafter.
init()
{
    sys := load Sys Sys->PATH;
    tasks := load Asinctasks Asinctasks->PATH;
    check(tasks != nil && sys != nil, "load real bytecode modules");
    owner = tasks->start(1);
    submitted := call(tasks->Submit, 0, 0, 0);
    check(submitted.status == tasks->Ok && submitted.task == 2147483647,
          "last positive signed ID is admitted");
    snapshot := call(tasks->Snapshot, 0, 0, 0);
    check(snapshot.status == tasks->Ok && len snapshot.tasks == 1 &&
          snapshot.tasks[0].id == 2147483647 && snapshot.tasks[0].inputHeld == 1,
          "maximum ID does not index the bounded array");
    check(call(tasks->Submit, 0, 0, 0).status == tasks->Full,
          "next Submit refuses instead of overflowing");
    claim := call(tasks->Claim, 0, 4, 0);
    check(claim.status == tasks->Ok && claim.task == 2147483647 && claim.lease == 2147483647,
          "maximum ID remains claimable");
    check(call(tasks->Retire, claim.task, 0, 0).status == tasks->Busy,
          "active maximum ID cannot be retired");
    check(call(tasks->Complete, claim.task, 4, claim.lease).status == tasks->Ok,
          "maximum ID completes normally");
    check(call(tasks->Retire, claim.task, 0, 0).status == tasks->Busy,
          "empty valid result must be taken first");
    check(call(tasks->TakeResult, claim.task, 0, 0).status == tasks->Ok,
          "maximum ID result transfers");
    check(call(tasks->Retire, claim.task, 0, 0).status == tasks->Ok,
          "maximum ID can be retired");
    snapshot = call(tasks->Snapshot, 0, 0, 0);
    check(snapshot.status == tasks->Ok && len snapshot.tasks == 0,
          "retirement frees all record storage");
    check(call(tasks->Submit, 0, 0, 0).status == tasks->Full,
          "empty table still refuses exhausted ID range");
    check(call(tasks->Claim, 0, 4, 0).status == tasks->Empty,
          "empty reused storage creates no phantom task");
    check(call(tasks->Complete, claim.task, 4, claim.lease).status == tasks->Stale,
          "maximum retired ID stays stale");
    check(call(tasks->Cancel, 0, 0, 0).status == tasks->Invalid &&
          call(tasks->Cancel, -2147483647 - 1, 0, 0).status == tasks->Invalid,
          "zero and negative identities remain invalid");
    check(call(tasks->Shutdown, 0, 0, 0).status == tasks->Ok,
          "quiescent exhausted owner can shut down");
    sys->print("PASS asinc identity limit: %d checks, no wrap, bounded indexing, retirement and stale maximum ID\n", checks);
}
