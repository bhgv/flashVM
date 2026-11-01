# Ordered raw native input acquisition, independent of replaceable vector fitting.


# Borrow a job owned by the supervisor and materialize a frozen native stroke once.
# Return status/retained. Preserve all earlier raw shapes/history; never vectorize
# here. Failure is sticky at the supervisor so Save cannot hide a missing stroke.
# [GC:] Frozen bytes become private raw geometry before another job is received.
acquire(vector: Idevector, job: ref Job, broken: int): (int,int)
{
    if (job.token == 0) {
        return (Idevector->Ok,!broken);
    }
    if (broken) {
        return (Idevector->Invalid,0);
    }
    host := load Idehost Idehost->PATH;
    codec := load Uibridge Uibridge->PATH;
    if (host == nil || codec == nil) {
        return (Idevector->Invalid,0);
    }
    bytes := array of byte host->ink(job.token);
    count := len bytes/8;
    if (count < 1 || count > Idevector->MaxNodes || len bytes%8 != 0) {
        return (Idevector->Invalid,0);
    }
    for (at := 0; at < len bytes; at += 8) {
        (ok,point) := codec->parseword(bytes,at);
        if (!ok || point < 0 || point > 601600) {
            return (Idevector->Invalid,0);
        }
    }
    job.document.revision = job.previous;
    for (index := 0; index < count; index++) {
        (unused,point) := codec->parseword(bytes,index*8);
        phase := 2;
        if (index == 0) {
            phase = 1;
        }
        status := vector->defer(job.document,phase,point%1001,point/1001);
        if (status != Idevector->Ok) {
            return (status,0);
        }
    }
    last := job.document.points[job.document.used-1];
    status := vector->defer(job.document,3,last.x,last.y);
    retained := job.document.revision == job.revision;
    if (status == Idevector->Ok && count == Idevector->MaxNodes) {
        status = Idevector->Limit;
    }
    return (status,retained);
}
