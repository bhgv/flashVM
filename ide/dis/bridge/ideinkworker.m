# Ordered raw handoff and latest-wins cancellable geometry; no UI heap is shared.
Ideinkworker: module
{
    PATH: con "ideinkworker.dis"; # Matching portable worker resource.
    Job: adt {
        revision: int; # Target committed generation.
        document: ref Idevector->Document; # Private state; immutable shapes may be shared.
        token: int; # Frozen native trajectory; zero for ordinary raw-path refinement.
        previous: int; # Exact predecessor for cumulative raw native capture.
    };
    Done: adt {
        revision: int; # Matching publication generation.
        status: int; # Explicit geometry/refinement outcome.
        document: ref Idevector->Document; # Immutable completion, including history.
        token: int; # Native preview publication fence, zero for ordinary jobs.
        retained: int; # Complete portable raw/result geometry exists; zero blocks Save.
        superseded: int; # Cumulative fitting stop requests; diagnostic only.
    };
    # New Job cancels the previous fit; native capture remains ordered/lossless.
    # Only latest Done publishes. Nil Job stops and drains private children.
    start: fn(vector: Idevector): (chan of ref Job, chan of ref Done);
    # Copy mutable history slots; share only immutable shape arrays, no live points.
    copy: fn(vector: Idevector, source: ref Idevector->Document): ref Idevector->Document;
    # Adopt a caller-verified matching geometry/history result; caller fences its outgoing wire.
    apply: fn(vector: Idevector, doc: ref Idevector->Document, done: ref Done);
    # Independent coalesced ready sender. Send1 to publish,0 to stop; no lost notice.
    notifier: fn(notices: chan of int, identity: int): chan of int;
};
