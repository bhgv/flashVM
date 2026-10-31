implement Idevector;
include "idevector.m";
include "uibridge.m";


# Allocate every scalar explicitly; arrays hold immutable path references only.
create(): ref Document
{
    return ref Document(nil, 0, 0, Brush, 16r245ac4, 8, 0, nil, 0,
                        array[16] of array of ref Shape, 0,
                        array[16] of array of ref Shape, 0);
}


# Bound both coordinates before storing any sample or imported command.
pointok(x, y: int): int
{
    return x >= 0 && x <= 1000 && y >= 0 && y <= 600;
}


# Form a closed rectangle or eight-quadratic ellipse from two bounded corners.
# Returned nodes are private immutable values. Ellipse approximation uses tangent
# intersections at 22.5 degrees and integer half-extents. Odd extents may
# shrink by one logical unit; all control points stay inside the requested box.
figure(tool: int, a, b: Point): array of Node
{
    if (tool == Line) {
        return array[] of {Node(Move, a.x, a.y, 0, 0), Node(Segment, b.x, b.y, 0, 0)};
    }
    left := a.x;
    right := b.x;
    top := a.y;
    bottom := b.y;
    if (left > right) {
        (left, right) = (right, left);
    }
    if (top > bottom) {
        (top, bottom) = (bottom, top);
    }
    if (tool == Rectangle) {
        return array[] of {Node(Move, left, top, 0, 0), Node(Segment, right, top, 0, 0),
            Node(Segment, right, bottom, 0, 0), Node(Segment, left, bottom, 0, 0), Node(Close, 0, 0, 0, 0)};
    }
    x := (left + right) / 2;
    y := (top + bottom) / 2;
    rx := (right - left) / 2;
    ry := (bottom - top) / 2;
    dx := rx * 707 / 1000;
    dy := ry * 707 / 1000;
    cx := rx * 414 / 1000;
    cy := ry * 414 / 1000;
    return array[] of {Node(Move, x + rx, y, 0, 0),
        Node(Quadratic, x + dx, y + dy, x + rx, y + cy),
        Node(Quadratic, x, y + ry, x + cx, y + ry),
        Node(Quadratic, x - dx, y + dy, x - cx, y + ry),
        Node(Quadratic, x - rx, y, x - rx, y + cy),
        Node(Quadratic, x - dx, y - dy, x - rx, y - cy),
        Node(Quadratic, x, y - ry, x - cx, y - ry),
        Node(Quadratic, x + dx, y - dy, x + cx, y - ry),
        Node(Quadratic, x + rx, y, x + rx, y - cy), Node(Close, 0, 0, 0, 0)};
}


# Convert active samples to an immutable raw polyline or primitive preview.
# No fitting or decimation occurs here; release refinement belongs to a worker.
draft(doc: ref Document): ref Shape
{
    points := doc.points[0:doc.used];
    if (doc.tool != Brush && doc.tool != Pen) {
        return ref Shape(doc.color, doc.width, doc.fill,
                         figure(doc.tool, points[0], points[len points - 1]), nil, 0);
    }
    if (len points == 1) {
        p := points[0];
        left := p.x - doc.width / 2;
        top := p.y - doc.width / 2;
        right := left + doc.width;
        bottom := top + doc.width;
        if (left < 0) {
            left = 0;
        }
        if (top < 0) {
            top = 0;
        }
        if (right > 1000) {
            right = 1000;
        }
        if (bottom > 600) {
            bottom = 600;
        }
        area: array of byte;
        if (doc.tool == Brush) {
            area = zerobytes(GridBytes);
            nib(area, p, doc.width);
        }
        tool := Ellipse;
        if (doc.width == 1) {
            tool = Rectangle;
            if (left == 1000) {
                left--;
            }
            if (top == 600) {
                top--;
            }
            right = left + 1;
            bottom = top + 1;
        }
        return ref Shape(doc.color, 0, 1,
            figure(tool, Point(left, top), Point(right, bottom)), area, doc.tool);
    }
    nodes := array[len points] of Node;
    nodes[0] = Node(Move, points[0].x, points[0].y, 0, 0);
    for (i := 1; i < len points; i++) {
        nodes[i] = Node(Segment, points[i].x, points[i].y, 0, 0);
    }
    return ref Shape(doc.color, doc.width, 0, nodes, nil, doc.tool);
}


# Return a private array of immutable path references, optionally with live preview.
# No path/node array escapes into mutable editor control.
visible(doc: ref Document): array of ref Shape
{
    count := len doc.shapes;
    extra := len doc.points != 0;
    shapes := array[count + extra] of ref Shape;
    if (count != 0) {
        shapes[0:] = doc.shapes;
    }
    if (extra) {
        shapes[count] = draft(doc);
    }
    return shapes;
}


# Keep the16 newest immutable history entries. Caller owns stack and depth.
push(stack: array of array of ref Shape, depth: int, shapes: array of ref Shape): int
{
    if (depth == len stack) {
        stack[0:] = stack[1:];
        depth--;
    }
    stack[depth] = shapes;
    return depth + 1;
}


# Drop the gesture without changing committed state or its revision.
cancel(doc: ref Document)
{
    doc.points = nil;
    doc.used = 0;
}


# Admit bounded samples and commit one complete gesture as one Undo unit.
# At sample capacity commit the retained raw trace rather than cancel visible ink.
advance(doc: ref Document, phase, x, y, smooth: int): int
{
    capacity := 0;
    if (doc == nil || phase < 1 || phase > 3 || !pointok(x, y)) {
        return Invalid;
    }
    if (doc.tool < Brush || doc.tool > Pen || doc.color < 0 || doc.color > 16rffffff ||
        doc.width < 1 || doc.width > 32 || (doc.fill != 0 && doc.fill != 1)) {
        return Invalid;
    }
    if (phase == 1) {
        if (len doc.points != 0 || len doc.shapes == MaxShapes || doc.revision == 2147483647) {
            return Conflict;
        }
        total := 0;
        for (i := 0; i < len doc.shapes; i++) {
            total += len doc.shapes[i].nodes;
        }
        # Reserve the maximum raw gesture budget before starting a new stroke.
        if (total > MaxTotal - MaxNodes) {
            return Limit;
        }
        doc.points = array[64] of Point;
        doc.points[0] = Point(x, y);
        doc.used = 1;
    } else {
        if (len doc.points == 0) {
            return Conflict;
        }
        previous := doc.points[doc.used - 1];
        if (previous.x != x || previous.y != y) {
            if (doc.tool != Brush && doc.tool != Pen) {
                doc.points[1] = Point(x, y);
                doc.used = 2;
            } else {
                if (doc.used == MaxNodes) {
                    capacity = 1;
                    phase = 3;
                } else {
                    if (doc.used == len doc.points) {
                        points := array[len doc.points * 2] of Point;
                        points[0:] = doc.points;
                        doc.points = points;
                    }
                    doc.points[doc.used++] = Point(x, y);
                }
            }
        }
    }
    if (phase == 3) {
        doc.nundo = push(doc.undo, doc.nundo, doc.shapes);
        doc.shapes = visible(doc);
        doc.nredo = 0;
        for (i := 0; i < len doc.redo; i++) {
            doc.redo[i] = nil;
        }
        doc.revision++;
        cancel(doc);
        if (smooth) {
            result := refine(doc);
            if (result != Ok) {
                return result;
            }
        }
    }
    if (capacity) {
        return Limit;
    }
    return Ok;
}


# Pure model entry: collect raw points, then refine once after release.
# Borrow the sole owner's document; return explicit admission/geometry status.
pointer(doc: ref Document, phase, x, y: int): int
{
    return advance(doc, phase, x, y, 1);
}


# Live pane entry: commit raw ink as one history unit without contour processing.
# Caller sends a separate immutable snapshot to its scheduled Limbo worker.
defer(doc: ref Document, phase, x, y: int): int
{
    return advance(doc, phase, x, y, 0);
}


# Move immutable snapshots between bounded stacks. Each admitted transition is
# a new document revision; saved revision is untouched, conservatively dirty.
history(doc: ref Document, redo: int): int
{
    if (doc == nil || len doc.points != 0 || doc.revision == 2147483647 || (redo != 0 && redo != 1)) {
        return 0;
    }
    if (redo) {
        if (doc.nredo == 0) {
            return 0;
        }
        doc.nundo = push(doc.undo, doc.nundo, doc.shapes);
        doc.shapes = doc.redo[--doc.nredo];
        doc.redo[doc.nredo] = nil;
    } else {
        if (doc.nundo == 0) {
            return 0;
        }
        doc.nredo = push(doc.redo, doc.nredo, doc.shapes);
        doc.shapes = doc.undo[--doc.nundo];
        doc.undo[doc.nundo] = nil;
    }
    doc.revision++;
    return 1;
}


# Serialize only scalar values. Each record has colour/width/fill/count, then
# five words per command. All arrays remain with this document's owning process.
wire(doc: ref Document): string
{
    shapes := visible(doc);
    total := 0;
    for (index := 0; index < len shapes; index++) {
        total += len shapes[index].nodes;
    }
    bytes := array[56 + 32 * len shapes + 40 * total] of byte;
    putword(bytes, 0, doc.tool);
    putword(bytes, 8, doc.color);
    putword(bytes, 16, doc.width);
    putword(bytes, 24, doc.fill);
    putword(bytes, 32, doc.nundo);
    putword(bytes, 40, doc.nredo);
    putword(bytes, 48, len shapes);
    at := 56;
    for (index = 0; index < len shapes; index++) {
        shape := shapes[index];
        putword(bytes, at, shape.color);
        putword(bytes, at + 8, shape.width);
        putword(bytes, at + 16, shape.fill);
        putword(bytes, at + 24, len shape.nodes);
        at += 32;
        for (part := 0; part < len shape.nodes; part++) {
            node := shape.nodes[part];
            putword(bytes, at, node.verb);
            putword(bytes, at + 8, node.x);
            putword(bytes, at + 16, node.y);
            putword(bytes, at + 24, node.cx);
            putword(bytes, at + 32, node.cy);
            at += 40;
        }
    }
    return string bytes;
}


# Write one copied uint32 value into a preflight-sized owner-private ASCII buffer.
# No allocation/module load/string concatenation occurs in this inner transport loop.
putword(bytes: array of byte, at, value: int)
{
    digits := "0123456789abcdef";
    for (digit := 0; digit < 8; digit++) {
        bytes[at + digit] = byte digits[(value >> (28 - digit * 4)) & 15];
    }
}


# Produce deterministic SVG using the same generic commands as actual MovieClip
# presentation. The result owns its text and references no external resources.
svg(doc: ref Document): string
{
    codec := load Uibridge Uibridge->PATH;
    if (codec == nil) {
        return "";
    }
    result := "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1000\" height=\"600\" viewBox=\"0 0 1000 600\">\n";
    for (i := 0; i < len doc.shapes; i++) {
        shape := doc.shapes[i];
        encoded := codec->hexword(shape.color);
        color := "#" + encoded[2:];
        fill := "none";
        if (shape.fill) {
            fill = color;
        }
        stroke := "none";
        thickness := "";
        if (shape.width != 0) {
            stroke = color;
            thickness = " stroke-width=\"" + string shape.width + "\"";
        }
        result += "  <path fill=\"" + fill + "\" stroke=\"" + stroke + "\"" + thickness + " d=\"";
        for (j := 0; j < len shape.nodes; j++) {
            n := shape.nodes[j];
            if (n.verb == Close) {
                result += "Z ";
            } else {
                if (n.verb == Move) {
                    result += "M ";
                } else if (n.verb == Segment) {
                    result += "L ";
                } else {
                    result += "Q " + string n.cx + " " + string n.cy + " ";
                }
                result += string n.x + " " + string n.y + " ";
            }
        }
        result += "\"/>\n";
    }
    return result + "</svg>\n";
}


# Private bounded lexical cursor; SVG is a data format, not a language compiler.
# Errors are sticky and parsing never executes script, loads entities or does I/O.
Reader: adt {
    text: string; # Borrowed immutable input.
    at: int; # Next scalar offset.
    bad: int; # Sticky malformed/unsupported-input marker.
};


# Skip only XML/number-list whitespace, not arbitrary control characters.
space(r: ref Reader)
{
    while (r.at < len r.text && (r.text[r.at] == ' ' || r.text[r.at] == '\t' ||
           r.text[r.at] == '\r' || r.text[r.at] == '\n')) {
        r.at++;
    }
}


# Skip inter-element XML whitespace/comments only. Comment contents are discarded
# as non-rendering metadata; internal double hyphens, invalid scalars and missing
# terminators reject the document. This helper is never used inside path data.
xmlspace(r: ref Reader)
{
    for (;;) {
        space(r);
        if (r.bad || r.at + 4 > len r.text || r.text[r.at:r.at+4] != "<!--") {
            return;
        }
        r.at += 4;
        ended := 0;
        while (r.at < len r.text) {
            if (r.at + 2 <= len r.text && r.text[r.at:r.at+2] == "--") {
                if (r.at + 3 <= len r.text && r.text[r.at+2] == '>') {
                    r.at += 3;
                    ended = 1;
                } else {
                    r.bad = 1;
                }
                break;
            }
            c := r.text[r.at++];
            if (c < 0 || c > 16r10ffff || (c >= 16rd800 && c <= 16rdfff) ||
                (c < 32 && c != '\t' && c != '\r' && c != '\n')) {
                r.bad = 1;
                break;
            }
        }
        if (!ended || r.bad) {
            r.bad = 1;
            return;
        }
    }
}


# Read an exactly representable unsigned integer <=4096. A decimal suffix is
# admitted only when every fractional digit is zero; no rounding/truncation,
# exponent, units or nonzero fractional geometry is accepted.
number(r: ref Reader): int
{
    space(r);
    start := r.at;
    value := 0;
    while (r.at < len r.text && r.text[r.at] >= '0' && r.text[r.at] <= '9') {
        value = value * 10 + r.text[r.at++] - '0';
        if (value > 4096) {
            r.bad = 1;
            return 0;
        }
    }
    if (r.at == start) {
        r.bad = 1;
    }
    if (r.at < len r.text && r.text[r.at] == '.') {
        r.at++;
        fraction := r.at;
        while (r.at < len r.text && r.text[r.at] >= '0' && r.text[r.at] <= '9') {
            if (r.text[r.at] != '0') {
                r.bad = 1;
            }
            r.at++;
        }
        if (fraction == r.at) {
            r.bad = 1;
        }
    }
    return value;
}


# Parse an entire integer attribute, preserving a sticky failure in the parent.
integer(value: string, parent: ref Reader): int
{
    r := ref Reader(value, 0, 0);
    n := number(r);
    space(r);
    if (r.bad || r.at != len value) {
        parent.bad = 1;
    }
    return n;
}


# Read one plain XML name. Names are bounded; namespaces/styles are not resolved.
name(r: ref Reader): string
{
    start := r.at;
    while (r.at < len r.text) {
        c := r.text[r.at];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == ':')) {
            break;
        }
        r.at++;
    }
    if (start == r.at || r.at - start > 32) {
        r.bad = 1;
    }
    return r.text[start:r.at];
}


# Read a strict start/self-close/end tag and at most16 unique quoted attributes.
# Return tag name (end tags begin '/'), name/value pairs and self-close flag.
# Unknown constructs/entities are rejected before any document publication.
tag(r: ref Reader): (string, array of string, int)
{
    xmlspace(r);
    attrs := array[32] of string;
    count := 0;
    if (r.at >= len r.text || r.text[r.at++] != '<') {
        r.bad = 1;
        return ("", nil, 0);
    }
    ending := "";
    if (r.at < len r.text && (r.text[r.at] == '/' || r.text[r.at] == '?')) {
        ending = r.text[r.at:r.at+1];
        r.at++;
    }
    label := ending + name(r);
    for (;;) {
        before := r.at;
        space(r);
        if (r.bad || r.at >= len r.text) {
            r.bad = 1;
            return ("", nil, 0);
        }
        if (r.text[r.at] == '>') {
            r.at++;
            return (label, attrs[0:count], 0);
        }
        if (r.text[r.at] == '?' && ending == "?" && r.at + 1 < len r.text && r.text[r.at+1] == '>') {
            r.at += 2;
            return (label, attrs[0:count], 1);
        }
        if (r.text[r.at] == '/' && ending == "" && r.at + 1 < len r.text && r.text[r.at+1] == '>') {
            r.at += 2;
            return (label, attrs[0:count], 1);
        }
        if (ending == "/" || before == r.at || count == len attrs) {
            r.bad = 1;
            return ("", nil, 0);
        }
        key := name(r);
        for (i := 0; i < count; i += 2) {
            if (attrs[i] == key) {
                r.bad = 1;
            }
        }
        space(r);
        if (r.bad || r.at >= len r.text || r.text[r.at++] != '=') {
            r.bad = 1;
            return ("", nil, 0);
        }
        space(r);
        if (r.at >= len r.text || (r.text[r.at] != '\"' && r.text[r.at] != '\'')) {
            r.bad = 1;
            return ("", nil, 0);
        }
        quote := r.text[r.at++];
        start := r.at;
        while (r.at < len r.text && r.text[r.at] != quote) {
            c := r.text[r.at++];
            if (c == '&' || c == '<' || c == '>' || c == 0 || r.at - start > MaxSVG) {
                r.bad = 1;
                return ("", nil, 0);
            }
        }
        if (r.at == len r.text) {
            r.bad = 1;
            return ("", nil, 0);
        }
        attrs[count++] = key;
        attrs[count++] = r.text[start:r.at++];
    }
}


# Return one unique parsed attribute or a caller-supplied default value.
attribute(attrs: array of string, key, fallback: string): string
{
    for (i := 0; i < len attrs; i += 2) {
        if (attrs[i] == key) {
            return attrs[i+1];
        }
    }
    return fallback;
}


# Admit RGB24 syntax only, with no CSS/style or colour normalization side effects.
colour(value: string, r: ref Reader): int
{
    if (len value != 7 || value[0] != '#') {
        r.bad = 1;
        return 0;
    }
    result := 0;
    for (i := 1; i < len value; i++) {
        c := value[i];
        digit := -1;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        }
        if (digit < 0) {
            r.bad = 1;
            return 0;
        }
        result = result * 16 + digit;
    }
    return result;
}


# Parse explicit absolute M/L/Q/Z commands. No implicit repeats/subpaths,
# transforms or unsupported commands are silently approximated.
pathdata(text: string, parent: ref Reader): array of Node
{
    r := ref Reader(text, 0, 0);
    nodes := array[MaxNodes] of Node;
    count := 0;
    closed := 0;
    for (;;) {
        space(r);
        if (r.bad || r.at == len text) {
            break;
        }
        if (count == MaxNodes) {
            r.bad = 1;
            break;
        }
        op := text[r.at++];
        verb := Segment;
        x := 0;
        y := 0;
        cx := 0;
        cy := 0;
        if (op == 'Z' && count >= 2 && !closed) {
            verb = Close;
            closed = 1;
        } else if (((count == 0 || closed) && op == 'M') || (count > 0 && !closed && (op == 'L' || op == 'Q'))) {
            if (op == 'M') {
                verb = Move;
                closed = 0;
            }
            if (op == 'Q') {
                verb = Quadratic;
                cx = number(r);
                cy = number(r);
            }
            x = number(r);
            y = number(r);
            if (!pointok(x, y) || !pointok(cx, cy)) {
                r.bad = 1;
            }
        } else {
            r.bad = 1;
        }
        nodes[count++] = Node(verb, x, y, cx, cy);
    }
    if (r.bad || count < 2) {
        parent.bad = 1;
        return nil;
    }
    return nodes[0:count];
}


# Admit only the fixed logical canvas, allowing harmless numeric whitespace and
# zero fractional suffixes. No scaling or lossy coordinate conversion occurs.
canvasbox(text: string, parent: ref Reader): int
{
    r := ref Reader(text, 0, 0);
    x := number(r);
    y := number(r);
    wide := number(r);
    high := number(r);
    space(r);
    if (r.bad || r.at != len text || x != 0 || y != 0 || wide != 1000 || high != 600) {
        parent.bad = 1;
        return 0;
    }
    return 1;
}


# Transactionally import a strict bounded SVG subset on a1000×600 canvas.
# All unsupported attributes/elements fail instead of disappearing on Save.
parse(text: string): (int, ref Document)
{
    if (len text > MaxSVG || len array of byte text > MaxSVG) {
        return (Limit, nil);
    }
    r := ref Reader(text, 0, 0);
    if (len text != 0 && text[0] == 16rfeff) {
        r.at++;
    }
    space(r);
    first := r.at;
    (root, attrs, short) := tag(r);
    if (root == "?xml") {
        if (first + 5 > len text || text[first:first+5] != "<?xml" || !short ||
            len attrs < 2 || attrs[0] != "version" || attrs[1] != "1.0") {
            return (Invalid, nil);
        }
        for (i := 0; i < len attrs; i += 2) {
            key := attrs[i];
            value := attrs[i+1];
            if ((key == "encoding" && (i != 2 || (value != "UTF-8" && value != "utf-8"))) ||
                (key == "standalone" && ((value != "yes" && value != "no") ||
                 (i != 2 && i != 4))) ||
                (key != "version" && key != "encoding" && key != "standalone")) {
                return (Invalid, nil);
            }
        }
        (root, attrs, short) = tag(r);
    }
    if (r.bad || root != "svg") {
        return (Invalid, nil);
    }
    for (i := 0; i < len attrs; i += 2) {
        key := attrs[i];
        if ((key == "xmlns" && attrs[i+1] != "http://www.w3.org/2000/svg") ||
            (key == "width" && integer(attrs[i+1], r) != 1000) ||
            (key == "height" && integer(attrs[i+1], r) != 600) ||
            (key == "viewBox" && !canvasbox(attrs[i+1], r)) ||
            (key != "xmlns" && key != "width" && key != "height" && key != "viewBox")) {
            return (Invalid, nil);
        }
    }
    if (r.bad) {
        return (Invalid, nil);
    }
    doc := create();
    if (short) {
        xmlspace(r);
        if (r.bad || r.at != len text) {
            return (Invalid, nil);
        }
        return (Ok, doc);
    }
    storage := array[MaxShapes] of ref Shape;
    count := 0;
    total := 0;
    for (;;) {
        (label, values, selfclose) := tag(r);
        if (r.bad) {
            return (Invalid, nil);
        }
        if (label == "/svg") {
            xmlspace(r);
            if (r.bad || len values != 0 || selfclose || r.at != len text) {
                return (Invalid, nil);
            }
            doc.shapes = storage[0:count];
            return (Ok, doc);
        }
        if (!selfclose || (label != "path" && label != "rect" && label != "ellipse" && label != "line")) {
            return (Invalid, nil);
        }
        if (count == MaxShapes) {
            return (Limit, nil);
        }
        allowed := "|fill|stroke|stroke-width|";
        if (label == "path") {
            allowed += "d|";
        } else if (label == "rect") {
            allowed += "x|y|width|height|";
        } else if (label == "ellipse") {
            allowed += "cx|cy|rx|ry|";
        } else {
            allowed += "x1|y1|x2|y2|";
        }
        for (i = 0; i < len values; i += 2) {
            needle := "|" + values[i] + "|";
            found := 0;
            for (j := 0; j + len needle <= len allowed; j++) {
                if (allowed[j:j+len needle] == needle) {
                    found = 1;
                }
            }
            if (!found) {
                return (Invalid, nil);
            }
        }
        fill := attribute(values, "fill", "#000000");
        stroke := attribute(values, "stroke", "none");
        rgb := 0;
        wide := 0;
        if (fill != "none") {
            rgb = colour(fill, r);
        }
        if (stroke != "none") {
            color := colour(stroke, r);
            if (fill != "none" && color != rgb) {
                r.bad = 1;
            }
            rgb = color;
            wide = integer(attribute(values, "stroke-width", "1"), r);
            if (wide < 1 || wide > 32) {
                r.bad = 1;
            }
        } else if (attribute(values, "stroke-width", "") != "") {
            r.bad = 1;
        }
        if (fill == "none" && stroke == "none") {
            r.bad = 1;
        }
        nodes: array of Node;
        if (label == "path") {
            nodes = pathdata(attribute(values, "d", ""), r);
        } else {
            a, b: Point;
            tool := Rectangle;
            if (label == "rect") {
                a = Point(integer(attribute(values, "x", "0"), r), integer(attribute(values, "y", "0"), r));
                b = Point(a.x + integer(attribute(values, "width", ""), r), a.y + integer(attribute(values, "height", ""), r));
            } else if (label == "ellipse") {
                tool = Ellipse;
                x := integer(attribute(values, "cx", ""), r);
                y := integer(attribute(values, "cy", ""), r);
                rx := integer(attribute(values, "rx", ""), r);
                ry := integer(attribute(values, "ry", ""), r);
                a = Point(x - rx, y - ry);
                b = Point(x + rx, y + ry);
            } else {
                tool = Line;
                a = Point(integer(attribute(values, "x1", ""), r), integer(attribute(values, "y1", ""), r));
                b = Point(integer(attribute(values, "x2", ""), r), integer(attribute(values, "y2", ""), r));
            }
            if (!pointok(a.x, a.y) || !pointok(b.x, b.y)) {
                r.bad = 1;
            }
            nodes = figure(tool, a, b);
        }
        if (r.bad) {
            return (Invalid, nil);
        }
        total += len nodes;
        if (total > MaxTotal) {
            return (Limit, nil);
        }
        storage[count++] = ref Shape(rgb, wide, fill != "none", nodes, nil, 0);
    }
}


# Bounded logical paint grid. Cell edges use the SVG canvas coordinates.
GridWidth: con 1000; # Raster authoring precision is one logical canvas unit.
GridHeight: con 600; # Paint never escapes the document page.
GridBytes: con 75000; # Packed immutable coverage, eight horizontal cells per byte.
RawContour: con 65536; # Explicit boundary complexity bound before approximation.


# Allocate private zeroed scalar storage using bounded doubling copies. Plain
# Limbo scalar arrays are uninitialized; mask bits must never inherit freed data.
# size is a positive internal byte count. Array copies run through Dis primitives,
# avoiding one interpreted assignment per cell of the full coverage/palette grid.
zerobytes(size: int): array of byte
{
    bytes := array[size] of byte;
    bytes[0] = byte 0;
    for (used := 1; used < size;) {
        next := used * 2;
        if (next > size) {
            next = size;
        }
        bytes[used:] = bytes[0:next - used];
        used = next;
    }
    return bytes;
}


# Read a borrowed packed area; outside the canvas is empty. No allocation/mutation.
covered(area: array of byte, x, y: int): int
{
    if (x < 0 || y < 0 || x >= GridWidth || y >= GridHeight) {
        return 0;
    }
    index := y * GridWidth + x;
    return (int area[index >> 3] & (1 << (index & 7))) != 0;
}


# Add one in-page cell to a private area. Out-of-page contributions are clipped.
cover(area: array of byte, x, y: int)
{
    if (x >= 0 && x < GridWidth && y >= 0 && y < GridHeight) {
        index := y * GridWidth + x;
        area[index >> 3] |= byte (1 << (index & 7));
    }
}


# Stamp a circular nib of integer diameter1..32 into caller-owned coverage.
# Pixel-centre sampling handles diameter1 and even diameters without empty dots.
nib(area: array of byte, p: Point, diameter: int)
{
    if (p.x == GridWidth) {
        p.x--;
    }
    if (p.y == GridHeight) {
        p.y--;
    }
    left := p.x - diameter / 2;
    top := p.y - diameter / 2;
    cx := 2 * left + diameter;
    cy := 2 * top + diameter;
    for (y := top; y < top + diameter; y++) {
        dy := 2 * y + 1 - cy;
        for (x := left; x < left + diameter; x++) {
            dx := 2 * x + 1 - cx;
            if (dx * dx + dy * dy <= diameter * diameter) {
                cover(area, x, y);
            }
        }
    }
}


# Sweep a nib along every major-axis cell between two copied samples, endpoints included.
# Interpolation fills fast pointer gaps; only bounded integer arithmetic is used.
sweep(area: array of byte, a, b: Point, diameter: int)
{
    dx := b.x - a.x;
    dy := b.y - a.y;
    steps := dx;
    if (steps < 0) {
        steps = -steps;
    }
    high := dy;
    if (high < 0) {
        high = -high;
    }
    if (high > steps) {
        steps = high;
    }
    if (steps == 0) {
        nib(area, a, diameter);
        return;
    }
    for (step := 0; step <= steps; step++) {
        nib(area, Point(a.x + dx * step / steps, a.y + dy * step / steps), diameter);
    }
}


# Private flattened edge; no mutable document or renderer handle is retained.
PaintEdge: adt
{
    a: Point; # Initial logical coordinate.
    b: Point; # Final logical coordinate.
};


# Rasterize immutable generic path geometry, or borrow its canonical brush coverage.
# Returned new masks are private; borrowed canonical masks are never written here.
# Curves use eight fixed subdivisions for union sampling; SVG geometry stays intact.
shapearea(shape: ref Shape): array of byte
{
    if (shape.area != nil) {
        return shape.area;
    }
    area := zerobytes(GridBytes);
    edges := array[len shape.nodes * 9 + 1] of PaintEdge;
    count := 0;
    start := Point(0, 0);
    previous := start;
    opened := 0;
    for (index := 0; index < len shape.nodes; index++) {
        node := shape.nodes[index];
        finish := Point(node.x, node.y);
        if (node.verb == Move) {
            if (opened && shape.fill) {
                edges[count++] = PaintEdge(previous, start);
            }
            start = finish;
            previous = start;
            opened = 1;
        } else if (node.verb == Quadratic) {
            origin := previous;
            for (part := 1; part <= 8; part++) {
                left := 8 - part;
                point := Point((left * left * origin.x + 2 * left * part * node.cx + part * part * finish.x) / 64,
                               (left * left * origin.y + 2 * left * part * node.cy + part * part * finish.y) / 64);
                edges[count++] = PaintEdge(previous, point);
                previous = point;
            }
        } else {
            if (node.verb == Close) {
                finish = start;
                opened = 0;
            }
            edges[count++] = PaintEdge(previous, finish);
            previous = finish;
        }
    }
    if (opened && shape.fill) {
        edges[count++] = PaintEdge(previous, start);
    }
    if (shape.width > 0) {
        for (index = 0; index < count; index++) {
            sweep(area, edges[index].a, edges[index].b, shape.width);
        }
    }
    if (!shape.fill) {
        return area;
    }
    crossings := array[count] of (int, int);
    for (y := 0; y < GridHeight; y++) {
        used := 0;
        scan := 2 * y + 1;
        for (index = 0; index < count; index++) {
            edge := edges[index];
            direction := 0;
            if (2 * edge.a.y <= scan && scan < 2 * edge.b.y) {
                direction = 1;
            } else if (2 * edge.b.y <= scan && scan < 2 * edge.a.y) {
                direction = -1;
            }
            if (direction != 0) {
                x := 2 * edge.a.x + (scan - 2 * edge.a.y) * (edge.b.x - edge.a.x) / (edge.b.y - edge.a.y);
                at := used;
                while (at > 0 && crossings[at - 1].t0 > x) {
                    crossings[at] = crossings[at - 1];
                    at--;
                }
                crossings[at] = (x, direction);
                used++;
            }
        }
        winding := 0;
        last := 0;
        for (index = 0; index < used; index++) {
            (x, direction) := crossings[index];
            if (winding != 0) {
                for (cell := last / 2; 2 * cell + 1 < x; cell++) {
                    cover(area, cell, y);
                }
            }
            winding += direction;
            last = x;
        }
    }
    return area;
}


# Test touching/overlapping canonical coverage, including diagonal cell contact.
# Inputs are borrowed immutable masks; return1 without changing either one.
touches(a, b: array of byte): int
{
    for (index := 0; index < GridBytes; index++) {
        bits := int a[index];
        if (bits == 0) {
            continue;
        }
        if ((bits & int b[index]) != 0) {
            return 1;
        }
        for (bit := 0; bit < 8; bit++) {
            if ((bits & (1 << bit)) == 0) {
                continue;
            }
            cell := index * 8 + bit;
            x := cell % GridWidth;
            y := cell / GridWidth;
            for (dy := -1; dy <= 1; dy++) {
                for (dx := -1; dx <= 1; dx++) {
                    if (covered(b, x + dx, y + dy)) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}


# Approximate a copied closed boundary within0.75 logical units, using a bounded
# iterative Douglas-Peucker stack. Preserve at least three vertices and orientation.
# Return the original array when approximation would collapse or reverse a small loop.
approximate(points: array of Point): array of Point
{
    count := len points;
    if (count < 5) {
        return points;
    }
    keep := zerobytes(count);
    keep[0] = keep[count - 1] = byte 1;
    stack := array[count * 2] of int;
    used := 2;
    stack[0] = 0;
    stack[1] = count - 1;
    while (used != 0) {
        last := stack[--used];
        first := stack[--used];
        a := points[first];
        b := points[last];
        dx := real (b.x - a.x);
        dy := real (b.y - a.y);
        length := dx * dx + dy * dy;
        worst := -1;
        distance := 0.5625;
        for (index := first + 1; index < last; index++) {
            px := real (points[index].x - a.x);
            py := real (points[index].y - a.y);
            value := px * px + py * py;
            if (length != 0.0) {
                projection := (px * dx + py * dy) / length;
                if (projection < 0.0) {
                    projection = 0.0;
                }
                if (projection > 1.0) {
                    projection = 1.0;
                }
                px -= projection * dx;
                py -= projection * dy;
                value = px * px + py * py;
            }
            if (value > distance) {
                worst = index;
                distance = value;
            }
        }
        if (worst >= 0) {
            keep[worst] = byte 1;
            stack[used++] = first;
            stack[used++] = worst;
            stack[used++] = worst;
            stack[used++] = last;
        }
    }
    result := array[count] of Point;
    used = 0;
    for (index := 0; index < count; index++) {
        if (keep[index] != byte 0) {
            result[used++] = points[index];
        }
    }
    if (used < 4) {
        return points;
    }
    before := 0;
    after := 0;
    for (index = 1; index < count; index++) {
        before += points[index - 1].x * points[index].y - points[index].x * points[index - 1].y;
    }
    for (index = 1; index < used; index++) {
        after += result[index - 1].x * result[index].y - result[index].x * result[index - 1].y;
    }
    if (after == 0 || (before < 0) != (after < 0)) {
        return points;
    }
    return result[0:used];
}


# Trace filled-cell edges with consistent winding. Separate loops and holes become
# explicit M/L/Z subpaths of one filled vector shape. No contour fitting runs in move.
# Return nil on explicit complexity limit; callers retain already committed raw ink.
contours(area: array of byte): array of Node
{
    stride := GridWidth + 1;
    edges := zerobytes(stride * (GridHeight + 1));
    seeds := array[RawContour] of int;
    nseeds := 0;
    for (index := 0; index < GridBytes; index++) {
        bits := int area[index];
        if (bits == 0) {
            continue;
        }
        for (bit := 0; bit < 8; bit++) {
            if ((bits & (1 << bit)) == 0) {
                continue;
            }
            cell := index * 8 + bit;
            x := cell % GridWidth;
            y := cell / GridWidth;
            if (!covered(area, x, y - 1)) {
                vertex := y * stride + x;
                if (edges[vertex] == byte 0) {
                    if (nseeds == len seeds) {
                        return nil;
                    }
                    seeds[nseeds++] = vertex;
                }
                edges[vertex] |= byte 1;
            }
            if (!covered(area, x + 1, y)) {
                vertex := y * stride + x + 1;
                if (edges[vertex] == byte 0) {
                    if (nseeds == len seeds) {
                        return nil;
                    }
                    seeds[nseeds++] = vertex;
                }
                edges[vertex] |= byte 2;
            }
            if (!covered(area, x, y + 1)) {
                vertex := (y + 1) * stride + x + 1;
                if (edges[vertex] == byte 0) {
                    if (nseeds == len seeds) {
                        return nil;
                    }
                    seeds[nseeds++] = vertex;
                }
                edges[vertex] |= byte 4;
            }
            if (!covered(area, x - 1, y)) {
                vertex := (y + 1) * stride + x;
                if (edges[vertex] == byte 0) {
                    if (nseeds == len seeds) {
                        return nil;
                    }
                    seeds[nseeds++] = vertex;
                }
                edges[vertex] |= byte 8;
            }
        }
    }
    nodes := array[MaxNodes] of Node;
    used := 0;
    points := array[RawContour] of Point;
    for (seed := 0; seed < nseeds; seed++) {
        vertex := seeds[seed];
        while (edges[vertex] != byte 0) {
            count := 1;
            points[0] = Point(vertex % stride, vertex / stride);
            at := vertex;
            direction := 0;
            while ((int edges[at] & (1 << direction)) == 0) {
                direction++;
            }
            for (;;) {
                edges[at] &= byte (255 ^ (1 << direction));
                x := at % stride;
                y := at / stride;
                if (direction == 0) {
                    x++;
                }
                if (direction == 1) {
                    y++;
                }
                if (direction == 2) {
                    x--;
                }
                if (direction == 3) {
                    y--;
                }
                next := Point(x, y);
                if (count > 1 &&
                    (points[count - 1].x - points[count - 2].x) * (next.y - points[count - 1].y) ==
                    (points[count - 1].y - points[count - 2].y) * (next.x - points[count - 1].x)) {
                    points[count - 1] = next;
                } else {
                    if (count == RawContour) {
                        return nil;
                    }
                    points[count++] = next;
                }
                at = y * stride + x;
                if (at == vertex) {
                    break;
                }
                found := 0;
                for (choice := 0; choice < 4; choice++) {
                    turn := 1;
                    if (choice == 1) {
                        turn = 0;
                    }
                    if (choice == 2) {
                        turn = 3;
                    }
                    if (choice == 3) {
                        turn = 2;
                    }
                    selected := (direction + turn) % 4;
                    if ((int edges[at] & (1 << selected)) != 0) {
                        direction = selected;
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    return nil;
                }
            }
            path := approximate(points[0:count]);
            if (len path < 4) {
                return nil;
            }
            if (used + len path > MaxNodes) {
                return nil;
            }
            nodes[used++] = Node(Move, path[0].x, path[0].y, 0, 0);
            for (index = 1; index < len path - 1; index++) {
                nodes[used++] = Node(Segment, path[index].x, path[index].y, 0, 0);
            }
            nodes[used++] = Node(Close, 0, 0, 0, 0);
        }
    }
    return nodes[0:used];
}


# Refine only a private worker snapshot. The owner publishes the returned shapes
# only for a matching revision. History is untouched; all old shapes remain immutable.
# Return Limit on contour budget failure, preserving the raw committed preview.
refineone(doc: ref Document): int
{
    count := len doc.shapes;
    if (count == 0) {
        return Ok;
    }
    last := doc.shapes[count - 1];
    if (last.author == Pen) {
        if (last.width == 0) {
            replacement := array[count] of ref Shape;
            replacement[0:] = doc.shapes;
            replacement[count - 1] = ref Shape(last.color, 0, last.fill, last.nodes, last.area, 0);
            doc.shapes = replacement;
            return Ok;
        }
        if (len last.nodes == MaxNodes) {
            return Limit;
        }
        nodes := array[len last.nodes + 1] of Node;
        nodes[0] = last.nodes[0];
        for (index := 1; index < len last.nodes; index++) {
            a := last.nodes[index - 1];
            b := last.nodes[index];
            nodes[index] = Node(Quadratic, (a.x + b.x) / 2, (a.y + b.y) / 2, a.x, a.y);
        }
        nodes[len last.nodes] = last.nodes[len last.nodes - 1];
        replacement := array[count] of ref Shape;
        replacement[0:] = doc.shapes;
        replacement[count - 1] = ref Shape(last.color, last.width, 0, nodes, nil, 0);
        doc.shapes = replacement;
        return Ok;
    }
    if (last.author != Brush) {
        return Ok;
    }
    area := shapearea(last);
    merged := array[count] of { * => 0 };
    candidates := array[count] of array of byte;
    palette := zerobytes(GridWidth * GridHeight);
    for (index := 0; index < count - 1; index++) {
        old := doc.shapes[index];
        source := shapearea(old);
        for (part := 0; part < GridBytes; part++) {
            bits := int source[part];
            if (bits == 0) {
                continue;
            }
            for (bit := 0; bit < 8; bit++) {
                if ((bits & (1 << bit)) != 0) {
                    palette[part * 8 + bit] = byte (index + 1);
                }
            }
        }
        if (old.color == last.color && (old.fill || old.author == Brush)) {
            candidates[index] = zerobytes(GridBytes);
        }
    }
    for (cell := 0; count > 1 && cell < len palette; cell++) {
        slot := int palette[cell] - 1;
        if (slot >= 0 && candidates[slot] != nil) {
            candidates[slot][cell >> 3] |= byte (1 << (cell & 7));
        }
    }
    # Copy before union; canonical old coverage belongs to immutable history.
    combined := zerobytes(GridBytes);
    combined[0:] = area;
    changed := 1;
    while (changed) {
        changed = 0;
        for (index = 0; index < count - 1; index++) {
            if (!merged[index] && candidates[index] != nil && touches(combined, candidates[index])) {
                merged[index] = 1;
                changed = 1;
                for (part := 0; part < GridBytes; part++) {
                    combined[part] |= candidates[index][part];
                }
            }
        }
    }
    nodes := contours(combined);
    if (len nodes < 2) {
        return Limit;
    }
    retained := 1;
    total := len nodes;
    for (index = 0; index < count - 1; index++) {
        if (!merged[index]) {
            retained++;
            total += len doc.shapes[index].nodes;
        }
    }
    if (retained > MaxShapes || total > MaxTotal) {
        return Limit;
    }
    replacement := array[retained] of ref Shape;
    used := 0;
    for (index = 0; index < count - 1; index++) {
        if (!merged[index]) {
            replacement[used++] = doc.shapes[index];
        }
    }
    replacement[used] = ref Shape(last.color, 0, 1, nodes, combined, 0);
    doc.shapes = replacement;
    return Ok;
}


# Refine every unprocessed raw gesture in creation order on a private snapshot.
# Latest-wins workers may supersede older jobs; no earlier stroke is then skipped.
# Failed bounded refinements leave their original visible ink and report Limit.
refine(doc: ref Document): int
{
    index := 0;
    status := Ok;
    while (index < len doc.shapes) {
        if (doc.shapes[index].author == 0) {
            index++;
            continue;
        }
        work := create();
        work.shapes = doc.shapes[0:index + 1];
        result := refineone(work);
        if (result != Ok) {
            status = result;
            index++;
            continue;
        }
        tail := len doc.shapes - index - 1;
        joined := array[len work.shapes + tail] of ref Shape;
        joined[0:] = work.shapes;
        if (tail != 0) {
            joined[len work.shapes:] = doc.shapes[index + 1:];
        }
        doc.shapes = joined;
        index = len work.shapes;
    }
    return status;
}
