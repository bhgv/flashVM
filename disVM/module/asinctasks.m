# Cooperative task lifetime owner; independent of frame publication and AS lowering.
Asinctasks: module
{
    PATH: con "asinctasks.dis"; # Ordinary portable Dis module.
    Capacity: con 32; # Maximum retained records; retired storage can be reused.
    Snapshot, # Detached inspection without changing ownership.
    Submit, # Create a queued scalar input.
    Claim, # Assign one queued task to one positive worker.
    Complete, # Finish with a successful scalar result.
    Fail, # Finish with a scalar error result.
    Cancel, # Request cancellation; running workers must acknowledge.
    AcknowledgeCancel, # Matching worker confirms it has stopped.
    TakeResult, # Transfer terminal result exactly once.
    Shutdown, # Refuse active workers; otherwise discard roots and exit.
    Retire: con iota; # Remove a terminal record only after its result was taken.
    Queued, # Owner retains input before any worker claims it.
    Running, # Claimed worker may still use input.
    CancelRequested, # Cancellation won; awaiting worker acknowledgement.
    Completed, # Successful terminal task, result optionally still retained.
    Failed, # Failed terminal task, error optionally still retained.
    Cancelled: con iota; # Terminal cancellation with no owner input/result.
    Ok, # Accepted request.
    Invalid, # Invalid command, identity, payload or state.
    Stale, # Wrong owner/worker/lease, or a previously issued but retired task ID.
    Full, # Retained record capacity or monotonic ID range exhausted.
    Empty, # Claim found no queued task.
    Terminal, # Task already terminal or cancelled without result.
    Busy, # Task/owner still has unfinished work.
    Taken: con iota; # Terminal result already transferred.


    # Scalar-only request. The caller must consume its reply before sending another.
    Request: adt
    {
        id: int; # Nonnegative reply correlation.
        incarnation: int; # Exact positive owner generation supplied to start.
        operation: int; # Command above.
        task: int; # Submit ignores this; other task commands require returned ID.
        worker: int; # Positive worker identity for Claim and finishing commands.
        lease: int; # Exact Claim token for finishing commands.
        value: string; # Immutable input/result/error, at most65536 characters.
        reply: chan of Reply; # Required synchronous acknowledgement channel.
    };


    # Detached task observation; empty strings alone do not imply absence of a root.
    Task: adt
    {
        id: int; # Monotonic positive task ID; never reused within an incarnation.
        state: int; # Current state constant.
        worker: int; # Claiming worker, zero before Claim.
        lease: int; # Unique claim token within this incarnation.
        input: string; # Owner-retained input while queued/running/cancelling.
        result: string; # Owner-retained terminal value until TakeResult.
        inputHeld: int; # Explicit owner input-root lifetime marker.
        resultHeld: int; # Explicit owner result-root lifetime marker.
    };


    # Claim returns value=input; TakeResult returns value=result after owner release.
    Reply: adt
    {
        id: int; # Request correlation.
        status: int; # Result code.
        task: int; # Submitted/claimed/requested task ID.
        lease: int; # Claim token, otherwise zero.
        value: string; # Detached immutable input or transferred result.
        tasks: array of Task; # Snapshot only; detached from private owner arrays.
    };

    # Return nil for invalid incarnation; otherwise spawn one scheduled Limbo owner.
    # Allocation failures propagate. Caller owns channel/replies and shutdown protocol.
    start: fn(incarnation: int): chan of Request;
};
