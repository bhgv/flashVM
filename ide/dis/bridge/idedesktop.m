# A typed Limbo owner for the MovieClip desktop, independent of rendering/OS pointers.
Idedesktop: module
{
    PATH: con "idedesktop.dis"; # Matched layout/gesture implementation.
    BeginView: con 65569; # width/height, ticket/count/scroll metadata.
    ItemView: con 65570; # Stable MovieClip ID and copied descriptor.
    EndView: con 65571; # Complete the exact declared scene.
    PointerEvent: con 65572; # Pixel/phase proposal carrying the completed view ticket.
    ResizeEvent: con 65573; # Ticket-bound native canvas proposal.
    MaxWindows: con 8; # Bounded independent window processes in this first shell.
    View, Update, Pointer, Scroll, Tool, Resize, Select, Cancel, Remove, Activate, TextHit, Shutdown: con iota; # FIFO operations.
    Ok, Invalid, Conflict: con iota; # Explicit layout/input admission.
    OpenFile, SaveFile, Compile, NewEditor, Debugger, Help, Quit, CloseEditor, Focus, CancelClose: con iota + 1; # Tool/activation actions.

    Request: adt
    {
        id: int; # Nonnegative correlation identity.
        operation: int; # Operation above.
        ticket: int; # Exact view for Pointer/Scroll/TextHit; zero for controller updates.
        window: int; # Update/Select/Activate/Remove/TextHit: 1..8 ID; Tool: action.
        x: int; # Pointer pixel X; Scroll delta in pixels; Resize width.
        y: int; # Pointer pixel Y; Resize height.
        phase: int; # Pointer begin/move/end: 1/2/3.
        title: string; # Update: at most 32 Unicode scalars.
        body: string; # Update: at most 200 Unicode scalars.
        caret: int; # Update: body-relative caret, -1 hides it.
        anchor: int; # Update: clipped body-relative anchor, -1 hides selection.
        reply: chan of Reply; # Required unique acknowledgement.
    };

    Item: adt
    {
        id: int; # Stable local MovieClip slot, 1..63; never a native object address.
        parent: int; # 0 stage, 1 scrolling table, 2 left rail, 3 right rail, 4 overlay.
        mirror: int; # Zero, or the source window ID for a live scaled MovieClip view.
        x: int; # Parent-local pixels.
        y: int; # Parent-local pixels, before table translation.
        width: int; # Positive local width.
        height: int; # Positive local height.
        style: int; # Bounded palette: paper/active/dark/help/muted.
        text: string; # Counted Unicode title/body, never script evaluated.
        caret: int; # Text-relative caret or -1; source ownership stays on Dis.
        anchor: int; # Text-relative clipped anchor or -1; no source authority.
    };

    Reply: adt
    {
        id: int; # Correlation identity.
        status: int; # Ok/Invalid/Conflict.
        ticket: int; # Monotonic layout epoch; stale gestures are cancelled.
        scroll: int; # Nonnegative table translation magnitude.
        focus: int; # Focused window ID.
        drawer: int; # Expanded tool or zero.
        action: int; # Tool/Focus/CloseEditor/CancelClose proposal; zero otherwise.
        items: array of ref Item; # Independent scene values for MovieClip presentation.
    };

    # Remove forgets presentation only after the application retires its source
    # owner. It rejects system slots 2/3 and cancels gestures; it never sends a
    # document Shutdown or discards source text. New Update begins at grid position.
    # Spawn a new layout/gesture owner; windows remain separately owned channels.
    # No native/AS objects are stored. Shutdown acknowledgement is terminal.
    start: fn(): chan of Request;
};
