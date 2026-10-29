# One independently scheduled Limbo owner per desktop window.
Idedeskpane: module
{
    PATH: con "idedeskpane.dis"; # Matching portable window controller.
    Editor, Console, Help: con iota + 1; # Actual document owner, mutable log, immutable help.
    Snapshot, Input, Open, Save, Build, Diagnostic, SetText, Shutdown: con iota; # FIFO operations.

    Request: adt
    {
        id: int; # Nonnegative correlation identity.
        operation: int; # Operation above.
        text: string; # Open/Save path or console replacement text.
        event: ref Uibridge->Message; # Input only: copied keyboard/text/focus observation.
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
        diagnostic: string; # Plain build/error metadata, bounded by the caller before UI publication.
    };

    # Create an exclusive process and ordinary channel. Editor owns a real Idecore
    # and optional compiler panel; other kinds own separate text. Return nil on
    # invalid kind/missing dependencies. No UI/native heap pointer escapes.
    start: fn(kind: int, path, text: string): chan of Request;
};
