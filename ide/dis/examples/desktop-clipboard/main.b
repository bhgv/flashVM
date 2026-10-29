implement ClipboardExample; # A portable client of real session/document owners.
include "sys.m"; # Native console output; no UI or filesystem request.
include "uibridge.m"; # Copied selection/revision proposals.
include "idedeskpane.m"; # Independent source documents and their private histories.
include "idedeskclip.m"; # Separate copied-value session clipboard owner.

ClipboardExample: module
{
    init: fn(); # Execute acknowledged Copy/Cut/Paste with Unicode and stale guards.
};


# Complete one borrowed pane exchange. text/event are immutable copied values;
# consume its private reply before issuing another operation or abandoning it.
# Shutdown is terminal; no caller may reuse that endpoint afterwards.
pane(owner: chan of Idedeskpane->Request, operation: int, text: string,
     event: ref Uibridge->Message): Idedeskpane->Reply
{
    replies := chan of Idedeskpane->Reply; # Only this request owns the acknowledgement.
    owner <-= Idedeskpane->Request(1, operation, text, event, replies); # FIFO admission.
    return <-replies; # Retain the owner through completion.
}


# Exchange one clipboard value. expected is an exact write generation; Get and
# Shutdown ignore it. A successful Put acknowledgement precedes every UI Cut.
clip(owner: chan of Idedeskclip->Request, operation, expected: int, text: string): Idedeskclip->Reply
{
    replies := chan of Idedeskclip->Reply; # Independent correlation channel.
    owner <-= Idedeskclip->Request(1, operation, expected, text, replies); # No native pointer.
    return <-replies; # A rejection returns the unchanged current snapshot.
}


# No parameters. Start two source owners and a separate session clipboard.
# Only generated private strings are changed: no file, system clipboard, GUI,
# external emu or compiler is accessed. Explicit failures preserve observability.
init()
{
    sys := load Sys Sys->PATH; # Use the actual runtime's console.
    panes := load Idedeskpane Idedeskpane->PATH; # Load matching document bytecode.
    clips := load Idedeskclip Idedeskclip->PATH; # Load the separate value service.
    if (sys == nil || panes == nil || clips == nil) { # Missing dependencies are errors.
        raise "fail: clipboard tutorial modules"; # Never replace services with a mock.
    }
    left := panes->start(panes->Editor, "", "aπ😀z"); # Owner A keeps full Unicode source.
    right := panes->start(panes->Editor, "", "target"); # Owner B has separate history/state.
    if (left == nil || right == nil) { # Do not send to missing channels.
        raise "fail: clipboard tutorial panes"; # Report startup failure.
    }
    board := clips->start(); # The initial clipboard is empty, generation zero.
    before := pane(left, panes->Snapshot, "", nil); # Actual source revision, not a guessed counter.
    pane(left, panes->Place, "", ref Uibridge->Message(Uibridge->Request, 0, 1, before.revision, 1, 0, "")); # Begin after a.
    pane(left, panes->Place, "", ref Uibridge->Message(Uibridge->Request, 0, 3, before.revision, 3, 0, "")); # Select π and 😀.
    exported := pane(left, panes->ExportSelection, "", nil); # Full selected value, not viewport text.
    stored := clip(board, clips->Get, 0, ""); # Capture the write generation.
    stored = clip(board, clips->Put, stored.generation, exported.transfer); # Acknowledge the copied value first.
    if (exported.status != 0 || stored.status != clips->Ok || stored.text != "π😀" || stored.bytes != 6) { # UTF-8 bytes differ from scalars.
        raise "fail: clipboard tutorial copy"; # No Cut is authorized after a failed Put.
    }
    rejected := clip(board, clips->Put, stored.generation - 1, "stale"); # Deliberately old write identity.
    if (rejected.status != clips->Conflict || rejected.text != "π😀") { # Rejection preserves the buffer.
        raise "fail: clipboard tutorial generation"; # No silent replacement is accepted.
    }
    cut := pane(left, panes->CutSelection, "", ref Uibridge->Message(Uibridge->Request, 0, 0, exported.revision,
                exported.rangefirst, exported.rangelast, "")); # Recheck source revision and exact selection after ACK.
    if (cut.status != 0 || cut.body != "az") { # Only the captured range is removed.
        raise "fail: clipboard tutorial cut"; # Keep source failure visible.
    }
    target := pane(right, panes->Snapshot, "", nil); # Owner B chooses its own insertion range.
    pasted := pane(right, panes->PasteSelection, stored.text, ref Uibridge->Message(Uibridge->Request, 0, 0,
                    target.revision, target.rangefirst, target.rangelast, "")); # One owner Replace.
    if (pasted.status != 0 || pasted.body != "π😀target" || pasted.rangefirst != 2 || pasted.rangelast != 2) { # Private caret follows inserted scalars.
        raise "fail: clipboard tutorial paste"; # No byte-offset caret or cross-owner mutation.
    }
    stale := pane(left, panes->CutSelection, "", ref Uibridge->Message(Uibridge->Request, 0, 0,
                    exported.revision, exported.rangefirst, exported.rangelast, "")); # Old geometry cannot delete again.
    if (stale.status == 0 || stale.body != "az") { # Source stays unchanged on stale admission.
        raise "fail: clipboard tutorial stale cut"; # Never repeat a destructive operation blindly.
    }
    invalid := clip(board, clips->Put, stored.generation, "bad\0value"); # NUL is outside the transfer contract.
    if (invalid.status != clips->Invalid || invalid.text != "π😀") { # Error preserves stored data.
        raise "fail: clipboard tutorial invalid text"; # No partial clipboard publication.
    }
    cleared := clip(board, clips->Clear, stored.generation, ""); # Explicitly release the retained session value.
    if (cleared.status != clips->Ok || cleared.text != "" || stored.text != "π😀") { # Older replies remain immutable copies.
        raise "fail: clipboard tutorial clear"; # Clearing cannot mutate a client's retained string.
    }
    pane(left, panes->Shutdown, "", nil); # Consume A's terminal acknowledgement.
    pane(right, panes->Shutdown, "", nil); # Consume B's independent terminal acknowledgement.
    clip(board, clips->Shutdown, 0, ""); # Release the separate clipboard owner last.
    sys->print("PASS desktop clipboard tutorial: Unicode copy/cut/paste, generation, stale source, immutable clear, shutdown\n"); # Actual service acceptance.
}
