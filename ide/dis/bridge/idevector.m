# Bounded immutable vector paths; mutable documents belong to one Limbo pane owner.
Idevector: module
{
    PATH: con "idevector.dis"; # Portable model/SVG implementation, no native UI dependency.
    Ok, Invalid, Limit, Conflict: con iota; # Success, bounded raw retention or explicit refusal.
    Brush, Rectangle, Ellipse, Line, Pen: con iota + 1; # Authoring tools; paths remain editable values.
    Move, Segment, Quadratic, Close: con iota; # Generic path verbs; quadratic has one control.
    MaxShapes: con 32; # Per-document path budget.
    MaxNodes: con 4096; # Per-path command budget.
    MaxSVG: con 1048576; # Complete SVG UTF-8 source bound, including comments.
    MaxTotal: con 16384; # Per-document complete preview/committed command budget.


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
        nodes: array of Node; # Owned complete ordered path, including closed compound contours.
        area: array of byte; # Immutable brush coverage, nil for imported/unprocessed paths.
        author: int; # Pending Brush/Pen origin; zero after processing/import/primitive creation.
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
        points: array of Point; # Private growing sample storage, nil when idle.
        used: int; # Initialized samples; no decimation or smoothing while drawing.
        undo: array of array of ref Shape; # Up to16 immutable prior states.
        nundo: int; # Initialized Undo stack size.
        redo: array of array of ref Shape; # Up to16 immutable subsequent states.
        nredo: int; # Initialized Redo stack size.
    };

    # Allocate a clean empty document and private history. No I/O or channel operation.
    create: fn(): ref Document;
    # Apply logical pointer phase1/2/3. Preserve samples and show raw ink until release.
    # Release commits one Undo unit, then refines Brush/Pen synchronously for pure clients.
    # At capacity, Limit commits the retained raw trace; no earlier points are erased.
    # Geometry Limit also leaves visible raw ink intact; live panes use the worker below.
    pointer: fn(doc: ref Document, phase, x, y: int): int;
    # Commit raw ink immediately; a caller-owned worker may refine its immutable snapshot.
    defer: fn(doc: ref Document, phase, x, y: int): int;
    # Refine every committed raw Brush/Pen in a private job document. No I/O/UI.
    refine: fn(doc: ref Document): int;
    # Drop only the active uncommitted gesture; source/history remain unchanged.
    cancel: fn(doc: ref Document);
    # Return0/1 for undo/redo admission. Gesture-active requests are refused.
    history: fn(doc: ref Document, redo: int): int;
    # Serialize a complete copied preview: six style/history words, path count,
    # then bounded ASCII path commands. The typed UI adapter requires Begin v2.
    wire: fn(doc: ref Document): string;
    # Serialize complete committed SVG. Callers refuse Save while a gesture is active.
    svg: fn(doc: ref Document): string;
    # Parse the documented strict SVG subset transactionally. On error return nil;
    # no existing document/file is modified and no external resource is loaded.
    parse: fn(text: string): (int, ref Document);
};
