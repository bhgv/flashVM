implement Idevector;
include "idevector.m";
include "uibridge.m";


# Allocate every scalar explicitly; arrays hold immutable path references only.
create(): ref Document
{
    return ref Document(nil, 0, 0, Brush, 16r245ac4, 8, 0, nil,
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


# Convert the active private samples into one immutable interpolated path.
# Midpoint quadratic joins share a tangent. No renderer/font/native object is used.
draft(doc: ref Document): ref Shape
{
    points := doc.points;
    if (doc.tool != Brush) {
        return ref Shape(doc.color, doc.width, doc.fill,
                         figure(doc.tool, points[0], points[len points - 1]));
    }
    if (len points == 1) {
        radius := (doc.width + 1) / 2;
        left := points[0].x - radius;
        top := points[0].y - radius;
        right := points[0].x + radius;
        bottom := points[0].y + radius;
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
        return ref Shape(doc.color, 0, 1, figure(Ellipse, Point(left, top), Point(right, bottom)));
    }
    nodes := array[len points + 1] of Node;
    nodes[0] = Node(Move, points[0].x, points[0].y, 0, 0);
    for (i := 1; i < len points; i++) {
        nodes[i] = Node(Quadratic, (points[i-1].x + points[i].x) / 2,
            (points[i-1].y + points[i].y) / 2, points[i-1].x, points[i-1].y);
    }
    last := points[len points - 1];
    nodes[len points] = Node(Segment, last.x, last.y, 0, 0);
    return ref Shape(doc.color, doc.width, 0, nodes);
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
}


# Admit bounded samples and commit one complete gesture as one Undo unit.
# Capacity decimation preserves endpoints but deliberately reduces fine detail.
pointer(doc: ref Document, phase, x, y: int): int
{
    if (doc == nil || phase < 1 || phase > 3 || !pointok(x, y)) {
        return Invalid;
    }
    if (doc.tool < Brush || doc.tool > Line || doc.color < 0 || doc.color > 16rffffff ||
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
        # Reserve enough space for any of the four authoring tools.
        if (total > MaxTotal - MaxNodes) {
            return Limit;
        }
        doc.points = array[] of {Point(x, y)};
    } else {
        if (len doc.points == 0) {
            return Conflict;
        }
        previous := doc.points[len doc.points - 1];
        if (previous.x != x || previous.y != y) {
            if (doc.tool == Brush && len doc.points == MaxNodes - 1) {
                reduced := array[(len doc.points + 1) / 2] of Point;
                for (i := 0; i < len reduced; i++) {
                    reduced[i] = doc.points[i * 2];
                }
                doc.points = reduced;
            }
            if (doc.tool != Brush) {
                first := doc.points[0];
                doc.points = array[] of {first, Point(x, y)};
            } else {
                points := array[len doc.points + 1] of Point;
                points[0:] = doc.points;
                points[len doc.points] = Point(x, y);
                doc.points = points;
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
    }
    return Ok;
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
    codec := load Uibridge Uibridge->PATH;
    if (codec == nil) {
        return "";
    }
    shapes := visible(doc);
    result := codec->hexword(doc.tool) + codec->hexword(doc.color) + codec->hexword(doc.width)
        + codec->hexword(doc.fill) + codec->hexword(doc.nundo) + codec->hexword(doc.nredo)
        + codec->hexword(len shapes);
    for (i := 0; i < len shapes; i++) {
        shape := shapes[i];
        result += codec->hexword(shape.color) + codec->hexword(shape.width)
            + codec->hexword(shape.fill) + codec->hexword(len shape.nodes);
        for (j := 0; j < len shape.nodes; j++) {
            n := shape.nodes[j];
            result += codec->hexword(n.verb) + codec->hexword(n.x) + codec->hexword(n.y)
                + codec->hexword(n.cx) + codec->hexword(n.cy);
        }
    }
    return result;
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
            if (c == '&' || c == '<' || c == '>' || c == 0 || r.at - start > 4096) {
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
        if (count == MaxNodes || closed) {
            r.bad = 1;
            break;
        }
        op := text[r.at++];
        verb := Segment;
        x := 0;
        y := 0;
        cx := 0;
        cy := 0;
        if (op == 'Z' && count >= 2) {
            verb = Close;
            closed = 1;
        } else if ((count == 0 && op == 'M') || (count > 0 && (op == 'L' || op == 'Q'))) {
            if (op == 'M') {
                verb = Move;
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
    if (len text > 65536 || len array of byte text > 65536) {
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
        storage[count++] = ref Shape(rgb, wide, fill != "none", nodes);
    }
}
