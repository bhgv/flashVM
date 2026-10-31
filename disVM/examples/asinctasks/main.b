implement TaskDemo;
include "sys.m";
include "asinctasks.m";
TaskDemo: module { init: fn(); }; # Portable typed-channel task lifetime example.


# Send one operation to generation1 and consume its acknowledgement.
# Caller supplies a live owner and task/worker/lease obtained from previous replies.
# Fail explicitly on refusal; production callers should handle each status themselves.
send(owner: chan of Asinctasks->Request, operation, task, worker, lease: int,
     value: string): Asinctasks->Reply
{
    replies := chan of Asinctasks->Reply;
    owner <-= Asinctasks->Request(1, 1, operation, task, worker, lease, value, replies);
    reply := <-replies;
    if (reply.status != Asinctasks->Ok) {
        raise "fail: task demo refused operation " + string operation;
    }
    return reply;
}


# Run one actual scheduled worker; it receives a scalar capture through Claim.
# The copy of input is independently rooted until this worker returns. Complete
# releases the service input and retains the result until the consumer takes it.
work(owner: chan of Asinctasks->Request, done: chan of int)
{
    claim := send(owner, Asinctasks->Claim, 0, 1, 0, "");
    send(owner, Asinctasks->Complete, claim.task, 1, claim.lease, claim.value + " / done");
    done <-= 1;
}


# Load ordinary Dis bytecode, submit, execute, take the result once, then shut down.
# No timeline/frame call or native thread is involved. This demonstrates the service
# foundation; it does not compile or execute the proposed ActionScript asinc syntax.
init()
{
    sys := load Sys Sys->PATH;
    tasks := load Asinctasks Asinctasks->PATH;
    if (sys == nil || tasks == nil) {
        raise "fail: task demo module load";
    }
    owner := tasks->start(1);
    submitted := send(owner, tasks->Submit, 0, 0, 0, "immutable capture");
    done := chan of int;
    spawn work(owner, done);
    <-done;
    result := send(owner, tasks->TakeResult, submitted.task, 0, 0, "");
    sys->print("PASS asinc task demo: %s\n", result.value);
    send(owner, tasks->Shutdown, 0, 0, 0, "");
}
