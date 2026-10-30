# Revision-bound lexical catalog shared by lazy colour/context/search consumers.
Idesyntax: module
{
    PATH: con "idesyntax.dis"; # Portable Limbo implementation, no display dependency.
    Limbo, ActionScript: con iota + 1; # Lexical profiles, not compiler acceptance.
    Ok, Invalid, TooLarge, Pending: con iota; # Complete result, failure or lazy work outstanding.
    Update, Snapshot, Shutdown: con iota; # Lazy-owner operations; exactly one reply each.
    MaxCharacters: con 65536; # Scalar budget for one immutable source snapshot.
    MaxBytes: con 262144; # UTF-8 scanner allocation bound.
    MaxTokens: con 8192; # Complete catalog bound; never return a partial success.


    # Half-open Unicode scalar interval; copied values owned by the result receiver.
    Token: adt {
        kind: int; # 0 punctuation, 1 name, 2 number, 3 literal, 4 keyword, 5 trivia.
        first: int; # Inclusive document scalar offset.
        last: int; # Exclusive document scalar offset.
    };


    # One immutable analysis job; callers coalesce edits before the next submission.
    Request: adt {
        ticket: int; # Positive owner correlation, returned unchanged.
        revision: int; # Nonnegative source revision, never interpreted as a view ticket.
        language: int; # Limbo or ActionScript profile.
        text: string; # Borrowed immutable snapshot; worker never edits the document.
        stop: int; # Nonzero requests acknowledged shutdown without analysis.
    };


    # Completed receiver-owned lexical catalog; the service never mutates delivered arrays.
    Result: adt {
        ticket: int; # Original correlation value.
        revision: int; # Original source revision.
        language: int; # Original profile.
        status: int; # Ok, Invalid or TooLarge.
        tokens: array of Token; # Receiver-owned ordered intervals; nil on failure.
        scanned: int; # Generated records visited on success, excluding EOF.
        reused: int; # Records reused from the private successful baseline.
        restart: int; # Scalar rescan start; full unchanged reuse reports source length.
    };


    # Serialized lazy-owner command; replies never wait for parsing on Update/Snapshot.
    Command: adt {
        operation: int; # Update replaces pending work; Snapshot queries; Shutdown drains.
        job: Request; # Update snapshot; ticket must strictly increase for each accepted submission.
        reply: chan of Result; # Required owned rendezvous; nil commands are ignored.
    };

    # Start a worker with private generated scanner instances. Returns job/result channels.
    # Nil channels report missing scanner dependencies. Each accepted job needs one receive;
    # stop also receives an acknowledgement. No frame clock, renderer or OS thread claim.
    start: fn(): (chan of Request, chan of Result);

    # Start a revision-aware lazy owner: one active scan and at most one newest snapshot.
    # notices may be a buffered channel; readiness delivery is best effort, Snapshot is authoritative.
    # Update/Snapshot acknowledge before scanner completion. Shutdown drains bounded work.
    # A new unique ticket is mandatory even when revision/profile equal an earlier request.
    startlazy: fn(notices: chan of int): chan of Command;

    # Project an authentic immutable catalog to <=200 ASCII palette bytes (classes 0..5).
    # Revision/profile mismatch or invalid viewport returns empty; no source/UI mutation.
    project: fn(result: Result, revision, language, first, count: int): string;
};
