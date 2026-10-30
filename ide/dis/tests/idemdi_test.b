implement MdiTest;
include "sys.m";
include "idemdi.m";
MdiTest: module { init: fn(); }; # Typed registry regression, no renderer simulation.


# Explicit oracle independent of optimization flags; inputs are copied values.
check(condition: int, message: string)
{
    if (!condition) {
        raise "fail: MDI " + message;
    }
}


# Borrow one owner through a complete synchronous request/acknowledgement pair.
call(owner: chan of Idemdi->Request, revision, operation, document, container, family: int): Idemdi->Reply
{
    replies := chan of Idemdi->Reply;
    owner <-= Idemdi->Request(1, revision, operation, document, container, family, replies);
    return <-replies;
}


# Verify source/graphics separation, SDI restoration, stale atomic refusal and
# independent returned arrays. No UI, file or target execution is fabricated.
init()
{
    sys := load Sys Sys->PATH;
    mdi := load Idemdi Idemdi->PATH;
    owner := mdi->start();
    initial := call(owner, 0, mdi->Snapshot, 0, 0, 0);
    source := call(owner, initial.revision, mdi->Register, 1, 0, mdi->Source);
    graphics := call(owner, source.revision, mdi->Register, 2, 0, mdi->Graphics);
    group := call(owner, graphics.revision, mdi->Create, 0, 1, mdi->Source);
    joined := call(owner, group.revision, mdi->Join, 1, 1, 0);
    check(joined.status == mdi->Ok && joined.containers[0] == 1, "source joins matching MDI");
    denied := call(owner, joined.revision, mdi->Join, 2, 1, 0);
    check(denied.status == mdi->Invalid && denied.revision == joined.revision && denied.containers[1] == 0,
          "graphics cannot inherit source tools");
    stale := call(owner, group.revision, mdi->Leave, 1, 0, 0);
    check(stale.status == mdi->Conflict && stale.containers[0] == 1, "stale detach is atomic");
    joined.containers[0] = 7;
    fresh := call(owner, 0, mdi->Snapshot, 0, 0, 0);
    check(fresh.containers[0] == 1, "reply arrays do not alias private membership");
    detached := call(owner, fresh.revision, mdi->Leave, 1, 0, 0);
    check(detached.containers[0] == 0 && detached.families[0] == mdi->Source,
          "SDI detach retains document identity and tools");
    joined = call(owner, detached.revision, mdi->Join, 1, 1, 0);
    dissolved := call(owner, joined.revision, mdi->Dissolve, 0, 1, 0);
    check(dissolved.containers[0] == 0 && dissolved.tools[0] == 0 && dissolved.families[0] == mdi->Source,
          "dissolve restores SDI without discarding source");
    check(call(owner, dissolved.revision, mdi->Shutdown, 0, 0, 0).status == mdi->Ok, "terminal acknowledgement");
    sys->print("PASS MDI membership: same desk, homogeneous tools, SDI detach, stale refusal and copied snapshots\n");
}
