# One independently scheduled Limbo owner per desktop window.
Idedeskpane: module
{
    PATH: con "idedeskpane.dis"; # Matching portable window controller.
    Editor, Console, Help, Vector, Debugger: con iota + 1; # Actual document owner, mutable log, immutable help.
    Snapshot, Input, Open, Save, Build, Diagnostic, SetText, Place, CancelPointer, ExportSelection, CutSelection, PasteSelection, Shutdown: con iota; # FIFO operations.

    VectorControl: con 14; # Revision-bound toolbar cell1..15; copied event code, no source coordinates.
    Draw: con 13; # Revision-bound logical canvas gesture; event code=phase, first/second=X/Y.

    Request: adt
    {
        id: int; # Nonnegative correlation identity.
        operation: int; # Operation above.
        text: string; # Open/Save path or console replacement text.
        # Place: Request kind, code begin/move/end=1/2/3, flags exact source revision,
        # first absolute scalar offset, second Shift 0/1 (begin only). Nil rejects.
        # CutSelection/PasteSelection require Request kind, flags=source revision,
        # first/second=the exact ordered full selection captured from a fresh reply.
        # Cut follows a successful session clipboard acknowledgement; Paste text is bounded.
        # CancelPointer drops the gesture without changing selection/modifiers.
        # Draw requires Request kind, code=phase1..3, flags=shown revision,
        # first/second=logical X/Y and empty text. VectorControl requires Request
        # kind, code1..15, flags=committed revision, zero coordinates/empty text
        # and an idle gesture. Both produce one ordinary pane acknowledgement.
        event: ref Uibridge->Message; # Input keyboard/text/focus, or a checked value proposal.
        reply: chan of Reply; # Required unique acknowledgement.
    };

    Reply: adt
    {
        id: int; # Original request ID.
        status: int; # Idecore status or mapped Invalid/Conflict.
        kind: int; # Window kind above.
        path: string; # Actual editor destination; empty for untitled/non-editor.
        body: string; # At most 200 Unicode scalars around the current caret.
        revision: int; # Actual document revision, zero for non-editor windows.
        saved: int; # Saved revision, never changed by Build or Snapshot.
        first: int; # Original source offset of body.
        caret: int; # Body-relative bounded caret.
        anchor: int; # Body-relative clipped selection anchor; full endpoint stays private.
        rangefirst: int; # Full ordered source selection start, not a clipped viewport coordinate.
        rangelast: int; # Full ordered source end, excluded.
        transfer: string; # ExportSelection only: complete bounded selection; never native UI metadata.
        line: int; # One-based logical source line containing the viewport start.
        diagnostic: string; # Plain build/error metadata, bounded by the caller before UI publication.
        drawing: string; # Vector v2 style/history header and complete bounded paths; empty for text panes.
        colors: string; # Empty while pending; otherwise one ASCII palette class 0..5 per body scalar.
    };

    # Create an exclusive process and ordinary channel. Editor owns a real Idecore
    # and optional compiler panel; other kinds own separate text. Return nil on
    # invalid kind/missing dependencies. No UI/native heap pointer escapes.
    start: fn(kind: int, path, text: string): chan of Request;

    # Same owner, with best-effort ready notifications carrying the stable pane identity.
    # A buffered notices channel and positive identity enable analysis. Nil/zero keeps
    # the plain owner used by start; missing optional scanners also keep plain text.
    # No document or native pointer is sent through readiness notifications.
    startnotified: fn(kind: int, path, text: string, notices: chan of int,
                      identity: int): chan of Request;
};
