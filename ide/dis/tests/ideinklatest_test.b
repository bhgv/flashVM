implement Ideinklatesttest;
include "sys.m";
include "idevector.m";
include "ideinkworker.m";
# Real model, scheduler and latest-only supervisor.
Ideinklatesttest: module
{
    init: fn(); # Execute isolated cancellation and preservation checks.
};
sys: Sys; # Sole output capability; no native ink/UI fixture is substituted.


# Keep predicates active in interpreter/JIT; a failure terminates this test process.
check(ok: int, label: string)
{
    if (!ok) {
        sys->print("FAIL latest fitting: %s\n",label);
        raise "fail: latest fitting";
    }
}


# Cancel only after the real fitting process reaches repeated checkpoints.
# The instruction bound is a deadlock control, not a wall-clock speed assertion.
stoprunning(work: ref Idevector->Work, acknowledged: chan of int)
{
    for (turn := 0; turn < 1000000 && work.polls < 2; turn++) {
        # Separate Dis scheduling must expose progress before this bound.
    }
    reached := work.polls >= 2;
    work.cancelled = 1;
    acknowledged <-= reached;
}


# Build immutable raw input, test cancellation before/during work, then submit
# three authoritative snapshots without consuming intermediate results. Only the
# latest may publish, and every earlier source stroke/Undo entry must survive.
init()
{
    sys = load Sys Sys->PATH;
    vector := load Idevector Idevector->PATH;
    worker := load Ideinkworker Ideinkworker->PATH;
    check(vector != nil && worker != nil,"matching modules");
    raw := vector->create();
    raw.width = 20;
    check(vector->defer(raw,1,100,100) == vector->Ok,"raw begin");
    check(vector->defer(raw,2,850,450) == vector->Ok,"raw motion");
    check(vector->defer(raw,3,900,500) == vector->Ok,"raw release");
    check(raw.shapes[0].author == vector->Brush,"motion/release retain unprocessed source");
    before := raw.shapes;
    cancelled := worker->copy(vector,raw);
    cancelled.work = ref Idevector->Work(1,0);
    check(vector->refine(cancelled) == vector->Cancelled && cancelled.shapes == before,
        "pre-cancelled fit preserves exact raw snapshot");
    running := worker->copy(vector,raw);
    running.work = ref Idevector->Work(0,0);
    acknowledged := chan of int;
    spawn stoprunning(running.work,acknowledged);
    result := vector->refine(running);
    reached := <-acknowledged;
    sys->print("Cancellation observed: reached=%d status=%d polls=%d same=%d\n",reached,result,running.work.polls,running.shapes == before);
    check(reached && result == vector->Cancelled && running.shapes == before,
        "in-flight checkpoints stop computation and discard partial geometry");
    latest := worker->copy(vector,raw);
    latest.color = 16r222222;
    vector->defer(latest,1,100,500);
    vector->defer(latest,3,200,450);
    newest := worker->copy(vector,latest);
    newest.color = 16re04444;
    newest.tool = vector->Pen;
    vector->defer(newest,1,700,80);
    vector->defer(newest,3,800,90);
    (jobs,completed) := worker->start(vector);
    jobs <-= ref Ideinkworker->Job(1,worker->copy(vector,raw),0,1);
    jobs <-= ref Ideinkworker->Job(2,worker->copy(vector,latest),0,2);
    jobs <-= ref Ideinkworker->Job(3,worker->copy(vector,newest),0,3);
    done := <-completed;
    check(done.revision == 3 && done.retained && done.status == vector->Ok && done.superseded >= 2,
        "new jobs cancel active work; only latest completion is published");
    check(len done.document.shapes == 3 && done.document.nundo == 3 &&
        done.document.shapes[0].color == 16r245ac4 && done.document.shapes[1].color == 16r222222 &&
        done.document.shapes[2].color == 16re04444,"cancellation loses no source stroke or history");
    jobs <-= nil;
    sys->print("PASS latest fitting: pre/in-flight cancellation, superseded results, complete raw input/history\n");
}
