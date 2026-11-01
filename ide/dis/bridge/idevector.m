# Bounded immutable vector paths; mutable documents belong to one Limbo pane owner.
Idevector: module
{
    PATH: con "idevector.dis"; # Portable model/SVG implementation, no native UI dependency.
    Ok, Invalid, Limit, Conflict: con iota; # Success, bounded raw retention or explicit refusal.
    Brush, Rectangle, Ellipse, Line, Pen, Select, Nodes, Zoom: con iota + 1; # Authoring tools; paths remain editable values.
    Move, Segment, Quadratic, Close, Cubic: con iota; # Path verbs; cubic has two independent controls.
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
        verb: int; # Move/Segment/Quadratic/Close/Cubic.
        x: int; # Destination X.
        y: int; # Destination Y.
        cx: int; # First Q/C control X; zero for straight verbs.
        cy: int; # First control Y.
        c2x: int; # Cubic second control X; zero for other verbs.
        c2y: int; # Cubic second control Y.
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


    # Private owner-local selection and gesture; immutable baselines support Cancel/Undo.
    Edit: adt {
        boundshape: ref Shape; # Exact immutable shape represented by boundbox.
        boundbox: array of int; # Cached left/top/right/bottom, owner-only.
        selected: int; # Selected shape index, -1 for none.
        mask: array of int; # Endpoint/control selection, three entries per command.
        base: array of ref Shape; # Original committed array for one active gesture.
        origin: Point; # Logical pointer-down position.
        mode: int; # 0 idle, 1 translate, 2 scale, 3 rotate, 4 nodes, 5 marquee.
        handle: int; # Corner ordinal for scaling.
        current: Point; # Last marquee position.
    };


    # Owner-only presentation cache; immutable source bytes are reused by reference.
    Presentation: adt {
        base: array of ref Shape; # Exact geometry identity represented by paths.
        tool: int; # Tool that produced the cached overlay.
        selected: int; # Selected source ordinal, -1 if none.
        dirty: int; # A pointer/mask change requires one overlay rebuild.
        paths: string; # Cached source body excluding the mutable shape-count word.
        overlay: string; # Cached transient path bytes, absent from SVG/history.
        count: string; # Copied shape-count word including the transient overlay.
        builds: int; # Saturating overlay rebuild diagnostic, never a source revision.
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
        wired: array of ref Shape; # Immutable committed identity represented by wirebody.
        wirebody: string; # Cached geometry bytes beginning with the shape count.
        nativecapture: int; # Native preview owns active pixels; reuse committed wire body on Begin.
        chain: array of Node; # Immutable unfinished Line vertices, nil after Finish/closure.
        zoom: int; # Canvas percent, 100 is fit/native page scale.
        center: Point; # Logical viewport centre.
        previous: int; # Authoring tool restored when Zoom toggles off.
        presentation: ref Presentation; # Owner-only selection wire cache.
        edit: ref Edit; # Selection never enters SVG/history or a render worker.
        control: int; # Copied Ctrl key state for additive node selection.
        wirecubic: int; # Cached body uses seven-word cubic-capable commands.
        pickradius: real; # Logical equivalent of20 screen pixels; portable default20.0.
        wirebuilds: int; # Saturating diagnostic count; never a document revision or wire field.
    };

    # Fit one bounded closed brush contour after release; nil refuses invalid input.
    # Pure worker/model helper; emits editable cubic controls and retained lines.
    brushfit: fn(points: array of Point, width, height: int): array of Node;
    # Allocate a clean empty document and private history. No I/O or channel operation.
    create: fn(): ref Document;
    # Sole owner/worker precomputes immutable committed wire bytes before handoff.
    preparewire: fn(doc: ref Document);
    # Apply logical pointer phase1/2/3. Preserve samples and show raw ink until release.
    # Release commits one Undo unit, then refines Brush/Pen synchronously for pure clients.
    # At capacity, Limit commits the retained raw trace; no earlier points are erased.
    # Geometry Limit also leaves visible raw ink intact; live panes use the worker below.
    pointer: fn(doc: ref Document, phase, x, y: int): int;
    # Commit raw ink immediately; a caller-owned worker may refine its immutable snapshot.
    defer: fn(doc: ref Document, phase, x, y: int): int;
    # Refine every committed raw Brush/Pen in a private job document. No I/O/UI.
    refine: fn(doc: ref Document): int;
    # Adjust viewport around a logical point; zero restores 1:1, no source/history edit.
    viewport: fn(doc: ref Document, step, x, y: int);
    # End an unfinished open line figure without closing/filling it. Pure owner call.
    finish: fn(doc: ref Document);
    # Drop only the active uncommitted gesture; source/history remain unchanged.
    cancel: fn(doc: ref Document);
    # Return0/1 for undo/redo admission. Gesture-active requests are refused.
    history: fn(doc: ref Document, redo: int): int;
    # Serialize a complete copied preview: six style/history words, path count,
    # then bounded ASCII path commands. The typed UI adapter requires Begin v2.
    wire: fn(doc: ref Document): string;
    # Return immutable stream segments: small current header + cached geometry
    # by reference. An active portable draft uses the legacy contiguous snapshot.
    wireparts: fn(doc: ref Document): array of string;
    # Serialize complete committed SVG. Callers refuse Save while a gesture is active.
    svg: fn(doc: ref Document): string;
    # Parse the documented strict SVG subset transactionally. On error return nil;
    # no existing document/file is modified and no external resource is loaded.
    parse: fn(text: string): (int, ref Document);
};
