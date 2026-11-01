# Cooperative cancellation for immutable fitting jobs, separate from input capture.


# Borrow a one-job capsule. Supervisor writes cancelled once; fitting checks it
# between bounded batches. Nil is the synchronous/pure-client path. No I/O occurs.
checkwork(work: ref Work)
{
    if (work != nil) {
        if (work.polls < 2147483647) {
            work.polls++;
        }
        if (work.cancelled) {
            raise "cancel: vector fitting";
        }
    }
}


# Refine private doc transactionally. A cancellation restores its exact input
# geometry and returns Cancelled; unrelated exceptions retain their diagnostics.
# The owner never receives this document until its matching completion is ready.
refine(doc: ref Document): int
{
    original := doc.shapes;
    {
        result := refinework(doc);
        checkwork(doc.work);
        return result;
    } exception failure {
        "cancel: vector fitting" =>
            doc.shapes = original;
            return Cancelled;
        * =>
            raise failure;
    }
}
