# Bounded immutable vector paths; mutable documents belong to one Limbo pane owner.
Idevector: module
{
    PATH: con "idevector.dis"; # Portable model/SVG implementation, no native UI dependency.
    Ok, Invalid, Limit, Conflict: con iota; # Complete success or explicit refusal.
    Brush, Rectangle, Ellipse, Line: con iota + 1; # Authoring tools; paths remain editable values.
    Move, Segment, Quadratic, Close: con iota; # Generic path verbs; quadratic has one control.
    MaxShapes: con 32; # Per-document path budget.
    MaxNodes: con 64; # Per-path command budget.
    MaxTotal: con 512; # Per-document complete preview/committed command budget.


    # Copied logical canvas point, 0..1000 horizontally and 0..600 vertically.
    Point: adt {
        x: int; # Logical X, independent of window zoom.
        y: int; # Logical Y.
    };


    # One immutable generic path command; Close uses zero coordinates.
    Node: adt {
        verb: int; # Move/Segment/Quadratic/Close.
        x: int; # Destination X.
        y: int; # Destination Y.
        cx: int; # Quadratic control X; zero for other verbs.
        cy: int; # Quadratic control Y.
    };


    # Published paths and node arrays must not be mutated after insertion/history capture.
    Shape: adt {
        color: int; # RGB24 shared by stroke and optional fill.
        width: int; # Logical stroke width,0 disables it; otherwise1..32.
        fill: int; # Zero for stroke only, one for same-colour fill.
        nodes: array of Node; # Owned complete ordered path.
    };


    # One pane-owned document; snapshots serialize values, never expose this reference.
    Document: adt {
        shapes: array of ref Shape; # Immutable committed path array.
        revision: int; # Monotonic edit/Undo/Redo revision.
        saved: int; # Revision last successfully written/read.
        tool: int; # Current authoring tool.
        color: int; # Current RGB24.
        width: int; # Current logical stroke width.
        fill: int; # Fill for closed authoring tools.
        points: array of Point; # Private active gesture samples, nil when idle.
        undo: array of array of ref Shape; # Up to16 immutable prior states.
        nundo: int; # Initialized Undo stack size.
        redo: array of array of ref Shape; # Up to16 immutable subsequent states.
        nredo: int; # Initialized Redo stack size.
    };

    # Allocate a clean empty document and private history. No I/O or channel operation.
    create: fn(): ref Document;
    # Apply logical pointer phase1/2/3. Repeated samples coalesce; brush samples
    # decimate at capacity and interpolate to one path per gesture. Invalid proposals
    # keep state. Successful release is one Undo unit; no boolean area union is implied.
    pointer: fn(doc: ref Document, phase, x, y: int): int;
    # Drop only the active uncommitted gesture; source/history remain unchanged.
    cancel: fn(doc: ref Document);
    # Return0/1 for undo/redo admission. Gesture-active requests are refused.
    history: fn(doc: ref Document, redo: int): int;
    # Serialize complete preview as bounded ASCII hex commands for the typed UI adapter.
    wire: fn(doc: ref Document): string;
    # Serialize complete committed SVG. Callers refuse Save while a gesture is active.
    svg: fn(doc: ref Document): string;
    # Parse the documented strict SVG subset transactionally. On error return nil;
    # no existing document/file is modified and no external resource is loaded.
    parse: fn(text: string): (int, ref Document);
};
