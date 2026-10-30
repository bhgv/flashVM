implement DebugTest;
include "sys.m";
include "idedebug.m";
DebugTest: module { init: fn(); }; # Actual disconnected session admission oracle.


# Fail explicitly in every optimization mode; condition/message are copied values.
check(condition: int, message: string)
{
    if (!condition) {
        raise "fail: debugger " + message;
    }
}


# Exchange one complete typed session command; owner is borrowed until reply.
call(owner: chan of Idedebug->Request, epoch, operation: int): Idedebug->Reply
{
    replies := chan of Idedebug->Reply;
    owner <-= Idedebug->Request(1, epoch, operation, replies);
    return <-replies;
}


# Verify honest unsupported controls, exact session incarnation and terminal
# shutdown without a target, native pointers, getter evaluation or test UI bypass.
init()
{
    sys := load Sys Sys->PATH;
    debug := load Idedebug Idedebug->PATH;
    check(debug != nil && debug->start(0) == nil, "invalid incarnation refused");
    owner := debug->start(7);
    initial := call(owner, 7, debug->Snapshot);
    check(initial.status == debug->Ok && initial.attached == 0 && initial.capabilities == 0,
          "disconnected capabilities explicit");
    for (operation := debug->Attach; operation <= debug->Next; operation++) {
        denied := call(owner, 7, operation);
        check(denied.status == debug->Unsupported && denied.source == initial.source &&
              denied.stack == initial.stack && denied.data == initial.data,
              "unsupported target operation preserves copied session state");
    }
    check(call(owner, 6, debug->Shutdown).status == debug->Stale,
          "old incarnation cannot terminate current session");
    check(call(owner, 7, -1).status == debug->Invalid, "unknown operation rejected");
    check(call(owner, 7, debug->Snapshot).status == debug->Ok, "refused shutdown kept session alive");
    check(call(owner, 7, debug->Shutdown).status == debug->Ok, "terminal acknowledgement");
    sys->print("PASS disconnected debugger: typed session, stale guards, unsupported controls and shutdown\n");
}
