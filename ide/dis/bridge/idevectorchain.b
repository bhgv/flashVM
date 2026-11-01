# Line figures belong to the document owner. Native input sends only endpoints;
# each release extends one immutable path, while the first segment owns Undo.


# End the current open figure. Existing vertices/history remain immutable;
# a later drag begins a separate figure. No blocking, callbacks or I/O.
finish(doc: ref Document)
{
    doc.chain = nil;
}


# Apply a checked Line phase and logical endpoint. Every drag after the first
# starts at the previous vertex. Release near the first vertex closes a polygon;
# only a closed polygon can use fill. Return the ordinary model status.
lineadvance(doc: ref Document, phase, x, y: int): int
{
    if (phase == 1) {
        if (len doc.points != 0 || doc.revision == 2147483647 ||
            (len doc.chain == 0 && len doc.shapes == MaxShapes)) {
            return Conflict;
        }
        if (len doc.chain >= MaxNodes - 2) {
            return Limit;
        }
        start := Point(x, y);
        if (len doc.chain != 0) {
            last := doc.chain[len doc.chain - 1];
            start = Point(last.x, last.y);
        }
        doc.points = array[] of {start, start};
        doc.used = 1;
        return Ok;
    }
    if (len doc.points == 0) {
        return Conflict;
    }
    doc.points[1] = Point(x, y);
    doc.used = 2;
    if (phase != 3) {
        return Ok;
    }
    previous := doc.chain;
    count := len previous;
    first := doc.points[0];
    if (count != 0) {
        first = Point(previous[0].x, previous[0].y);
    }
    dx := x - first.x;
    dy := y - first.y;
    closed := count >= 3 && dx * dx + dy * dy <= 64;
    if (count == 0) {
        count = 1;
    }
    nodes := array[count + 1 + closed] of Node;
    if (len previous != 0) {
        nodes[0:] = previous;
    } else {
        nodes[0] = Node(Move, first.x, first.y, 0, 0);
    }
    if (closed) {
        x = first.x;
        y = first.y;
        nodes[len nodes - 1] = Node(Close, 0, 0, 0, 0);
    }
    nodes[count] = Node(Segment, x, y, 0, 0);
    shape := ref Shape(doc.color, 1, closed && doc.fill, nodes, nil, 0);
    extra := len previous == 0;
    shapes := array[len doc.shapes + extra] of ref Shape;
    shapes[0:] = doc.shapes;
    shapes[len shapes - 1] = shape;
    {
        total := len nodes;
        for (i := 0; i < len doc.shapes; i++) {
            if (extra || i != len doc.shapes - 1) {
                total += len doc.shapes[i].nodes;
            }
        }
        if (total > MaxTotal) {
            return Limit;
        }
        if (extra) {
            doc.nundo = push(doc.undo, doc.nundo, doc.shapes);
        }
    }
    doc.shapes = shapes;
    doc.chain = nodes;
    if (closed) {
        finish(doc);
    }
    doc.nredo = 0;
    for (i := 0; i < len doc.redo; i++) {
        doc.redo[i] = nil;
    }
    doc.revision++;
    cancel(doc);
    return Ok;
}


# Serialize a transient extension in place of the last unfinished path, so an
# active segment never consumes a second shape slot or mutates committed nodes.
# Borrow the owner document; the temporary document and arrays stay private.
# No I/O/channel operation occurs, and the original committed wire cache survives.
chainwire(doc: ref Document): string
{
    preview := create();
    preview.tool = doc.tool;
    preview.color = doc.color;
    preview.width = doc.width;
    preview.fill = doc.fill;
    preview.chain = doc.chain;
    preview.nundo = doc.nundo;
    preview.nredo = doc.nredo;
    preview.shapes = array[len doc.shapes] of ref Shape;
    preview.shapes[0:] = doc.shapes;
    nodes := array[len doc.chain + 1] of Node;
    nodes[0:] = doc.chain;
    last := doc.points[doc.used - 1];
    nodes[len nodes - 1] = Node(Segment, last.x, last.y, 0, 0);
    preview.shapes[len preview.shapes - 1] = ref Shape(doc.color, 1, 0, nodes, nil, 0);
    return wire(preview);
}
