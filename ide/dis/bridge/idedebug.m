# Dis-owned debugger presentation/session scaffold; no implicit target attachment.
Idedebug: module
{
    PATH: con "idedebug.dis"; # Matched independently scheduled session owner.
    Snapshot, Attach, Pause, Continue, Step, Next, Shutdown: con iota; # Commands, exactly one reply.
    Ok, Invalid, Stale, Unsupported: con iota; # Explicit admission; Unsupported never changes target state.


    # Copied session command. The unique reply must be consumed before reuse;
    # no mutable target object, stack address or interpreter register escapes.
    Request: adt
    {
        id: int; # Nonnegative correlation identity.
        epoch: int; # Exact positive session incarnation.
        operation: int; # Command above.
        reply: chan of Reply; # Required acknowledgement; nil requests ignored.
    };


    # Immutable presentation snapshot. Empty capabilities prohibit run controls;
    # unavailable data is explicit, never substituted with sample target frames.
    Reply: adt
    {
        id: int; # Original correlation identity.
        epoch: int; # Current incarnation, even for rejected stale requests.
        status: int; # Admission status above.
        attached: int; # Zero until an actual authorized runtime target is integrated.
        capabilities: int; # Zero: no attach/step/next/pause/continue capability yet.
        source: string; # Read-only source/location panel.
        stack: string; # Read-only target stack panel, unavailable while disconnected.
        data: string; # Read-only locals/module panel; no getter/expression evaluation.
    };

    # Start an isolated disconnected session with positive epoch; nil on invalid.
    # Shutdown acknowledges then terminates. This is a UI/service scaffold, not
    # target execution, a mock /prog filesystem or a native debug capability.
    start: fn(epoch: int): chan of Request;
};
