implement PointerExample; # Portable client; no FlashVM geometry is fabricated here.
include "sys.m"; # Console output belongs to the native runtime module.
include "uibridge.m"; # Copied input/selection values, not object addresses.
include "idedeskpane.m"; # The pane owns a real private Idecore document.

PointerExample: module
{
    init: fn(); # Demonstrate current-revision selection and stale rejection.
};


# Send one copied operation to a borrowed owner and consume its private reply.
# event is nil or a typed proposal. This call blocks until acknowledgement;
# Shutdown is terminal, so the caller must never reuse that endpoint afterwards.
request(owner: chan of Idedeskpane->Request, operation: int,
        event: ref Uibridge->Message): Idedeskpane->Reply
{
    replies := chan of Idedeskpane->Reply; # One outstanding correlation at a time.
    owner <-= Idedeskpane->Request(1, operation, "", event, replies); # FIFO admission.
    return <-replies; # Retain ownership until the exact admitted call completes.
}


# No parameters. All offsets count Unicode scalars in the complete source.
# The client explicitly supplies proposals; a graphical application must first
# validate its real view ticket and glyph-hit identity through layout channels.
# Only generated strings are edited. No source file, renderer or compiler is used.
init()
{
    sys := load Sys Sys->PATH; # Resolve real runtime console support.
    panes := load Idedeskpane Idedeskpane->PATH; # Load matching typed bytecode.
    if (sys == nil || panes == nil) { # Missing services are observable failures.
        raise "fail: pointer tutorial dependencies"; # Never substitute a local text model.
    }
    owner := panes->start(panes->Editor, "", "aπ😀z"); # Start one real document process.
    if (owner == nil) { # Do not send to a missing channel.
        raise "fail: pointer tutorial startup"; # Report dependency/allocation failure.
    }
    initial := request(owner, panes->Snapshot, nil); # Capture the actual source revision.
    placed := request(owner, panes->Place, ref Uibridge->Message(Uibridge->Request, 0, 1, initial.revision, 1, 0, "")); # Begin at scalar one.
    placed = request(owner, panes->Place, ref Uibridge->Message(Uibridge->Request, 0, 2, initial.revision, 3, 0, "")); # Extend through π and 😀.
    placed = request(owner, panes->Place, ref Uibridge->Message(Uibridge->Request, 0, 3, initial.revision, 3, 0, "")); # End and retire the gesture.
    if (placed.status != 0 || placed.anchor != 1 || placed.caret != 3 || placed.revision != initial.revision) { # Selection is not an edit.
        raise "fail: pointer tutorial scalar selection"; # UTF-8 bytes are not source offsets.
    }
    edited := request(owner, panes->Input, ref Uibridge->Message(Uibridge->Text, 0, 0, 0, 0, 0, "X")); # Replace the private selection atomically.
    if (edited.body != "aXz" || edited.revision <= initial.revision) { # The owner performs the actual source mutation.
        raise "fail: pointer tutorial replacement"; # No clipped viewport replacement is allowed.
    }
    rejected := request(owner, panes->Place, ref Uibridge->Message(Uibridge->Request, 0, 1, initial.revision, 0, 0, "")); # Deliberately stale revision.
    if (rejected.status == 0 || rejected.body != edited.body || rejected.caret != edited.caret) { # Rejection preserves source and selection.
        raise "fail: pointer tutorial stale admission"; # Never accept old geometry for new source.
    }
    request(owner, panes->CancelPointer, nil); # Explicit cancellation is harmless after rejection.
    stopped := request(owner, panes->Shutdown, nil); # Consume the terminal owner acknowledgement.
    if (stopped.status != 0) { # Cleanup must complete before the success marker.
        raise "fail: pointer tutorial shutdown"; # A stopped channel is never reused.
    }
    sys->print("PASS desktop pointer tutorial: Unicode selection, owner edit, stale rejection, shutdown\n"); # Real service result.
}
