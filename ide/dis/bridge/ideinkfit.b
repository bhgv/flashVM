# One private cancellable fitting child; never receives input or controls a window.


# Own doc and its one-job cancellation capsule. Return exactly one private child
# reply even on refusal/exception, allowing the supervisor to drain cancelled work.
# Only the supervisor may publish a matching latest result to the pane owner.
fit(vector: Idevector, doc: ref Idevector->Document, revision, token, acquired, superseded: int,
    completed: chan of ref Done)
{
    original := doc.shapes;
    status := acquired;
    {
        if (acquired == Idevector->Ok || acquired == Idevector->Limit) {
            status = vector->refine(doc);
            if (status == Idevector->Ok && acquired == Idevector->Limit) {
                status = acquired;
            }
        }
        if (!doc.work.cancelled && status != Idevector->Cancelled) {
            vector->preparewire(doc);
        }
    } exception failure {
        "cancel: vector fitting" =>
            status = Idevector->Cancelled;
        * =>
            doc.shapes = original;
            status = Idevector->Limit;
    }
    if (doc.work.cancelled) {
        status = Idevector->Cancelled;
    }
    completed <-= ref Done(revision,status,doc,token,1,superseded);
}
