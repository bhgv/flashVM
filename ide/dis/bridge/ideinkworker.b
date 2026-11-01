implement Ideinkworker;
include "idevector.m";
include "uibridge.m";
include "idehost.m";
include "ideinkworker.m";


# Own ordered raw input separately from replaceable fitting jobs. Each new job
# stops the preceding fit and drops any pending old publication. Capture/defer is
# never discarded: carried retains every admitted native stroke and Undo entry.
# [GC:] Child documents/history are private; only immutable shapes and a monotonic
# cancellation capsule are shared. Stop cancels children and drains their replies.
worker(vector: Idevector, jobs: chan of ref Job, completed: chan of ref Done)
{
    fitted := chan of ref Done;
    parkedjobs := chan of ref Job;
    parkedout := chan of ref Done;
    carried: ref Idevector->Document;
    carriedrevision := -1;
    current := -1;
    active: ref Idevector->Work;
    pending: ref Done;
    children := 0;
    stopping := 0;
    broken := 0;
    superseded := 0;
    for (;;) {
        if (stopping && children == 0) {
            return;
        }
        outgoing := parkedout;
        if (pending != nil && !stopping) {
            outgoing = completed;
        }
        alt {
        job := <-jobs =>
            if (active != nil) {
                active.cancelled = 1;
                active = nil;
                if (superseded < 2147483647) {
                    superseded++;
                }
            }
            pending = nil;
            if (job == nil) {
                stopping = 1;
                jobs = parkedjobs;
                continue;
            }
            current = job.revision;
            if (job.token != 0 && carried != nil && carriedrevision == job.previous) {
                latest := copy(vector,carried);
                latest.tool = job.document.tool;
                latest.color = job.document.color;
                latest.width = job.document.width;
                latest.fill = job.document.fill;
                job.document = latest;
            }
            status := Idevector->Invalid;
            retained := 0;
            {
                (status,retained) = acquire(vector,job,broken);
            } exception failure {
                * =>
                    status = Idevector->Invalid;
                    retained = 0;
            }
            if (!retained) {
                broken = 1;
            }
            carried = job.document;
            carriedrevision = current;
            if (broken) {
                pending = ref Done(current,status,carried,job.token,0,superseded);
                continue;
            }
            snapshot := copy(vector,carried);
            active = ref Idevector->Work(0,0);
            snapshot.work = active;
            children++;
            spawn fit(vector,snapshot,job.revision,job.token,status,superseded,fitted);
        done := <-fitted =>
            children--;
            if (!stopping && done.revision == current && !done.document.work.cancelled &&
                done.status != Idevector->Cancelled) {
                active = nil;
                pending = done;
            }
        outgoing <-= pending =>
            if (pending.retained) {
                carried = copy(vector,pending.document);
                carriedrevision = pending.revision;
            }
            pending = nil;
        }
    }
}


include "ideinkacquire.b";
include "ideinkfit.b";


# Clone the mutable history containers, sharing only immutable geometry arrays.
# No active point array is transferred; native jobs acquire frozen points later.
copy(vector: Idevector, source: ref Idevector->Document): ref Idevector->Document
{
    copy := vector->create();
    copy.shapes = source.shapes;
    copy.revision = source.revision;
    copy.saved = source.saved;
    copy.tool = source.tool;
    copy.color = source.color;
    copy.width = source.width;
    copy.fill = source.fill;
    copy.chain = source.chain;
    copy.undo[0:] = source.undo;
    copy.nundo = source.nundo;
    copy.redo[0:] = source.redo;
    copy.nredo = source.nredo;
    return copy;
}


# Apply only a matching completion. Native worker history contains the ordered
# captured strokes; never share a mutable history slot array with that worker.
apply(vector: Idevector, doc: ref Idevector->Document, done: ref Done)
{
    doc.shapes = done.document.shapes;
    if (done.document.wired == done.document.shapes) {
        doc.wired = done.document.wired;
        doc.wirebody = done.document.wirebody;
        doc.wirecubic = done.document.wirecubic;
    }

    if (done.token != 0) {
        doc.undo[0:] = done.document.undo;
        doc.nundo = done.document.nundo;
        doc.redo[0:] = done.document.redo;
        doc.nredo = done.document.nredo;
    }
}


# Create one geometry process and transfer only private Job/Done capabilities.
start(vector: Idevector): (chan of ref Job, chan of ref Done)
{
    jobs := chan of ref Job;
    completed := chan of ref Done;
    spawn worker(vector, jobs, completed);
    return (jobs, completed);
}


# Keep ready state while the controller is busy; wait only on commands when
# idle. Stop remains admitted even when the controller no longer receives notices.
notify(commands, notices: chan of int, identity: int)
{
    pending := 0;
    for (;;) {
        if (!pending) {
            command := <-commands;
            if (command == 0) {
                return;
            }
            pending = 1;
            continue;
        }
        alt {
        command := <-commands =>
            if (command == 0) {
                return;
            }
            pending = 1;
        notices <-= identity =>
            pending = 0;
        }
    }
}


# Allocate one coalescing notifier process; all arguments/commands are values.
notifier(notices: chan of int, identity: int): chan of int
{
    commands := chan of int;
    spawn notify(commands, notices, identity);
    return commands;
}
