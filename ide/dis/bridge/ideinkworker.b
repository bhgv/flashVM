implement Ideinkworker;
include "idevector.m";
include "uibridge.m";
include "idehost.m";
include "ideinkworker.m";


# Own the sole geometry job until completion; no window/Flash object is accessed.
# Input ends with nil. Results rendezvous with the document owner; bounded FIFO
# jobs preserve every captured stroke. Drop transport roots before the next receive.
worker(vector: Idevector, jobs: chan of ref Job, completed: chan of ref Done)
{
    host: Idehost;
    codec := load Uibridge Uibridge->PATH;
    carried: ref Idevector->Document;
    carriedrevision := -1;
    broken := 0; # Never discard a preceding failed native stroke via a later completion.
    for (;;) {
        job := <-jobs;
        if (job == nil) {
            return;
        }
        status := Idevector->Invalid;
        if (job.token != 0 && carried != nil && carriedrevision == job.previous) {
            latest := copy(vector, carried);
            latest.tool = job.document.tool;
            latest.color = job.document.color;
            latest.width = job.document.width;
            latest.fill = job.document.fill;
            job.document = latest;
        }
        original := job.document.shapes;
        atcapacity := 0;
        {
            if (job.token != 0) {
                if (host == nil) {
                    host = load Idehost Idehost->PATH;
                }
                bytes: array of byte;
                if (host != nil) {
                    bytes = array of byte host->ink(job.token);
                }
                count := len bytes / 8;
                atcapacity = count == Idevector->MaxNodes;
                good := !broken && codec != nil && count >= 1 && count <= Idevector->MaxNodes && len bytes % 8 == 0;
                for (at := 0; good && at < len bytes; at += 8) {
                    (ok, point) := codec->parseword(bytes, at);
                    good = ok && point >= 0 && point <= 601600;
                }
                if (good) {
                    job.document.revision = job.previous;
                    for (index := 0; index < count; index++) {
                        (unused, point) := codec->parseword(bytes, index * 8);
                        phase := 2;
                        if (index == 0) {
                            phase = 1;
                        }
                        status = vector->defer(job.document, phase, point % 1001, point / 1001);
                        if (status != Idevector->Ok) {
                            break;
                        }
                    }
                    if (status == Idevector->Ok) {
                        last := job.document.points[job.document.used - 1];
                        status = vector->defer(job.document, 3, last.x, last.y);
                    }
                }
            } else {
                status = Idevector->Ok;
            }
            if (status == Idevector->Ok) {
                original = job.document.shapes;
                status = vector->refine(job.document);
                if (status == Idevector->Ok && atcapacity) {
                    status = Idevector->Limit;
                }
            }
            # Serialization is geometry work too. Hand immutable committed bytes
            # to the UI owner with the shapes, rather than blocking its Snapshot.
            vector->preparewire(job.document);
        } exception failure {
            * =>
                # [GC:] Recover only the private job; preserve the raw gesture
                # array so Save/Shutdown can receive a result rather than hang.
                job.document.shapes = original;
                status = Idevector->Limit;
        }
        retained := job.token == 0 || (!broken && job.document.revision == job.revision);
        if (!retained) {
            broken = 1;
        }
        carried = job.document;
        carriedrevision = job.revision;
        completed <-= ref Done(job.revision, status, job.document, job.token, retained);
        job = nil;
    }
}


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
    }

    if (done.token != 0) {
        doc.undo[0:] = done.document.undo;
        doc.nundo = done.document.nundo;
        doc.redo[0:] = done.document.redo;
        doc.nredo = done.document.nredo;
    }
}


# Keep every bounded native gesture in FIFO order. Earlier strokes cannot be
# discarded by latest-wins refinement scheduling. Caller limits the queue to31.
append(queue: list of ref Job, job: ref Job): list of ref Job
{
    if (queue == nil) {
        return job :: nil;
    }
    return hd queue :: append(tl queue, job);
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
