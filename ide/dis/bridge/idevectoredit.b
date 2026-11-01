# Owner-local immutable contour editing. No UI, I/O or shared worker mutation.


# Return conservative bounds including quadratic controls for one borrowed shape.
editbounds(shape: ref Shape): (int, int, int, int)
{
    left := 1000;
    top := 600;
    right := 0;
    bottom := 0;
    for (i := 0; i < len shape.nodes; i++) {
        n := shape.nodes[i];
        if (n.verb == Close) {
            continue;
        }
        if (n.x < left) {
            left = n.x;
        }
        if (n.y < top) {
            top = n.y;
        }
        if (n.x > right) {
            right = n.x;
        }
        if (n.y > bottom) {
            bottom = n.y;
        }
        if (n.verb == Quadratic) {
            if (n.cx < left) {
                left = n.cx;
            }
            if (n.cy < top) {
                top = n.cy;
            }
            if (n.cx > right) {
                right = n.cx;
            }
            if (n.cy > bottom) {
                bottom = n.cy;
            }
        }
    }
    return (left, top, right, bottom);
}


# Squared distance admission uses bounded logical coordinates; returns a hit.
editnear(x, y, a, b, radius: int): int
{
    return (x-a)*(x-a)+(y-b)*(y-b) <= radius*radius;
}


# Hit a flattened contour and filled interior. Quadratics use 32 bounded samples;
# source commands stay exact and are never replaced by these picking segments.
edithit(shape: ref Shape, x, y: int): int
{
    previous := Point(0, 0);
    start := previous;
    crossings := 0;
    for (i := 0; i < len shape.nodes; i++) {
        n := shape.nodes[i];
        if (n.verb == Move) {
            previous = Point(n.x, n.y);
            start = previous;
            continue;
        }
        end := Point(n.x, n.y);
        if (n.verb == Close) {
            end = start;
        }
        begin := previous;
        steps := 1;
        if (n.verb == Quadratic) {
            steps = 32;
        }
        for (step := 1; step <= steps; step++) {
            next := end;
            if (steps > 1) {
                t := step;
                u := steps-t;
                next = Point((u*u*begin.x+2*u*t*n.cx+t*t*end.x)/(steps*steps),
                             (u*u*begin.y+2*u*t*n.cy+t*t*end.y)/(steps*steps));
            }
            dx := next.x-previous.x;
            dy := next.y-previous.y;
            length := dx*dx+dy*dy;
            along := (x-previous.x)*dx+(y-previous.y)*dy;
            px := previous.x;
            py := previous.y;
            if (length > 0) {
                if (along < 0) {
                    along = 0;
                }
                if (along > length) {
                    along = length;
                }
                px += dx*along/length;
                py += dy*along/length;
            }
            if (editnear(x, y, px, py, 6+shape.width/2)) {
                return 1;
            }
            if ((previous.y > y) != (next.y > y)) {
                if (x < previous.x+(y-previous.y)*(next.x-previous.x)/(next.y-previous.y)) {
                    crossings++;
                }
            }
            previous = next;
        }
    }
    return shape.fill && (crossings & 1);
}


# Copy transformed geometry from the down snapshot, never from the previous move.
# Reject out-of-canvas results as a whole; preserve curve verbs and Undo baseline.
editmove(doc: ref Document, x, y: int): int
{
    e := doc.edit;
    shape := e.base[e.selected];
    (left, top, right, bottom) := editbounds(shape);
    dx := x-e.origin.x;
    dy := y-e.origin.y;
    ax := left;
    ay := top;
    bx := right;
    by := bottom;
    if (!(e.handle & 1)) {
        (ax, bx) = (bx, ax);
    }
    if (!(e.handle & 2)) {
        (ay, by) = (by, ay);
    }
    cosine := 1.0;
    sine := 0.0;
    centerx := real (left+right)/2.0;
    centery := real (top+bottom)/2.0;
    if (e.mode == 3) {
        math := load Math Math->PATH;
        if (math == nil) {
            return Invalid;
        }
        angle := math->atan2(real y-centery, real x-centerx)-
                 math->atan2(real e.origin.y-centery, real e.origin.x-centerx);
        cosine = math->cos(angle);
        sine = math->sin(angle);
    }
    changed := 0;
    nodes := array[len shape.nodes] of Node;
    for (i := 0; i < len nodes; i++) {
        n := shape.nodes[i];
        for (control := 0; control < 2 && n.verb != Close; control++) {
            if (control && n.verb != Quadratic) {
                continue;
            }
            px := n.x;
            py := n.y;
            if (control) {
                (px, py) = (n.cx, n.cy);
            }
            if (e.mode == 1 || (e.mode == 4 && e.mask[2*i+control])) {
                px += dx;
                py += dy;
            } else if (e.mode == 2) {
                if (bx != ax) {
                    px = ax+(px-ax)*(bx+dx-ax)/(bx-ax);
                }
                if (by != ay) {
                    py = ay+(py-ay)*(by+dy-ay)/(by-ay);
                }
            } else if (e.mode == 3) {
                tx := real px-centerx;
                ty := real py-centery;
                px = int (centerx+tx*cosine-ty*sine);
                py = int (centery+tx*sine+ty*cosine);
            }
            if (!pointok(px, py)) {
                return Invalid;
            }
            if (control) {
                (n.cx, n.cy) = (px, py);
            }
            else {
                (n.x, n.y) = (px, py);
            }
        }
        old := shape.nodes[i];
        if (n.x != old.x || n.y != old.y || n.cx != old.cx || n.cy != old.cy) {
            changed = 1;
        }
        nodes[i] = n;
    }
    if (!changed) {
        doc.shapes = e.base;
        return Ok;
    }
    result := array[len e.base] of ref Shape;
    result[0:] = e.base;
    result[e.selected] = ref Shape(shape.color, shape.width, shape.fill, nodes, nil, 0);
    doc.shapes = result;
    return Ok;
}


# Admit selection/marquee/transform phases. Selection is transient; only changed
# geometry commits one revision and one Undo item on release. Cancel restores base.
editpointer(doc: ref Document, phase, x, y: int): int
{
    if (doc.edit == nil) {
        doc.edit = ref Edit(-1, nil, nil, Point(0, 0), 0, 0, Point(0, 0));
    }
    e := doc.edit;
    if (phase == 1) {
        if (e.mode != 0 || doc.revision == 2147483647) {
            return Conflict;
        }
        finish(doc);
        e.origin = e.current = Point(x, y);
        e.mode = 0;
        if (e.selected >= len doc.shapes) {
            e.selected = -1;
        }
        if (e.selected >= 0) {
            shape := doc.shapes[e.selected];
            if (len e.mask != len shape.nodes*2) {
                e.mask = editmask(len shape.nodes*2);
            }
            if (doc.tool == Select) {
                (l, t, r, b) := editbounds(shape);
                for (h := 0; h < 4; h++) {
                    hx := l;
                    hy := t;
                    if (h & 1) {
                        hx = r;
                    }
                    if (h & 2) {
                        hy = b;
                    }
                    if (editnear(x, y, hx, hy, 9)) {
                        e.mode = 2;
                        e.handle = h;
                    }
                }
                if (editnear(x, y, (l+r)/2, t-18, 9)) {
                    e.mode = 3;
                }
            } else {
                hit := -1;
                best := 65;
                for (i := 0; i < len shape.nodes; i++) {
                    n := shape.nodes[i];
                    if (n.verb == Close) {
                        continue;
                    }
                    distance := (x-n.x)*(x-n.x)+(y-n.y)*(y-n.y);
                    if (distance <= 64 && distance <= best) {
                        hit = i*2;
                        best = distance;
                    }
                    if (n.verb == Quadratic) {
                        distance = (x-n.cx)*(x-n.cx)+(y-n.cy)*(y-n.cy);
                        if (distance <= 64 && distance <= best) {
                            hit = i*2+1;
                            best = distance;
                        }
                    }
                }
                if (hit >= 0) {
                    if (!doc.control && !e.mask[hit]) {
                        e.mask = editmask(len shape.nodes*2);
                    }
                    if (doc.control) {
                        e.mask[hit] = !e.mask[hit];
                    }
                    else {
                        e.mask[hit] = 1;
                    }
                    e.mode = 4;
                }
            }
        }
        if (e.mode == 0) {
            found := -1;
            for (i := len doc.shapes-1; i >= 0; i--) {
                if (edithit(doc.shapes[i], x, y)) {
                    found = i;
                    break;
                }
            }
            if (doc.tool == Select) {
                e.selected = found;
                if (found >= 0) {
                    e.mode = 1;
                }
            } else if (found >= 0 && found != e.selected) {
                e.selected = found;
                e.mask = editmask(2*len doc.shapes[found].nodes);
            } else if (e.selected >= 0) {
                if (!doc.control) {
                    e.mask = editmask(len e.mask);
                }
                e.mode = 5;
            }
        }
        e.base = doc.shapes;
        doc.points = array[] of {Point(x, y)};
        return Ok;
    }
    if (len doc.points == 0) {
        return Conflict;
    }
    e.current = Point(x, y);
    if (e.mode > 0 && e.mode < 5) {
        editmove(doc, x, y);
    }
    if (phase == 3) {
        if (e.mode == 5 && e.selected >= 0) {
            nodes := doc.shapes[e.selected].nodes;
            for (i := 0; i < len nodes; i++) {
                n := nodes[i];
                for (c := 0; c < 2 && n.verb != Close; c++) {
                    if (c && n.verb != Quadratic) {
                        continue;
                    }
                    px := n.x;
                    py := n.y;
                    if (c) {
                        (px, py) = (n.cx, n.cy);
                    }
                    if ((px-e.origin.x)*(px-x) <= 0 && (py-e.origin.y)*(py-y) <= 0) {
                        e.mask[2*i+c] = 1;
                    }
                }
            }
        }
        if (doc.shapes != e.base) {
            doc.nundo = push(doc.undo, doc.nundo, e.base);
            doc.nredo = 0;
            doc.revision++;
        }
        e.mode = 0;
        e.base = nil;
        doc.points = nil;
    }
    return Ok;
}


# Append one bounded square handle or frame as a subpath in caller-owned storage.
editbox(nodes: array of Node, at, l, t, r, b: int): int
{
    if (l < 0) {
        l = 0;
    }
    if (t < 0) {
        t = 0;
    }
    if (r > 1000) {
        r = 1000;
    }
    if (b > 600) {
        b = 600;
    }
    nodes[at:] = array[] of {Node(Move,l,t,0,0), Node(Segment,r,t,0,0),
        Node(Segment,r,b,0,0), Node(Segment,l,b,0,0), Node(Close,0,0,0,0)};
    return at+5;
}


# Return one temporary blue overlay, excluded from saved SVG and Undo arrays.
# Handles and curve control guides use the same logical coordinate system as ink.
editoverlay(doc: ref Document): ref Shape
{
    if (doc.tool == Zoom && len doc.points == 2) {
        nodes := array[5] of Node;
        a := doc.points[0];
        b := doc.points[1];
        if (a.x > b.x) {
            (a.x,b.x) = (b.x,a.x);
        }
        if (a.y > b.y) {
            (a.y,b.y) = (b.y,a.y);
        }
        editbox(nodes,0,a.x,a.y,b.x,b.y);
        return ref Shape(16r1687df,1,0,nodes,nil,0);
    }
    if (doc.tool == Zoom) {
        return nil;
    }
    e := doc.edit;
    if (e == nil || e.selected < 0 || e.selected >= len doc.shapes) {
        return nil;
    }
    shape := doc.shapes[e.selected];
    nodes := array[MaxNodes] of Node;
    at := 0;
    if (doc.tool == Select) {
        (l,t,r,b) := editbounds(shape);
        at = editbox(nodes,at,l,t,r,b);
        for (h := 0; h < 4; h++) {
            x := l;
            y := t;
            if (h & 1) {
                x = r;
            }
            if (h & 2) {
                y = b;
            }
            at = editbox(nodes,at,x-4,y-4,x+4,y+4);
        }
        at = editbox(nodes,at,(l+r)/2-4,t-22,(l+r)/2+4,t-14);
    } else if (doc.tool == Nodes) {
        for (part := 0; part < len shape.nodes && at < MaxNodes; part++) {
            nodes[at++] = shape.nodes[part];
        }
        for (i := 0; i < len shape.nodes && at+15 < MaxNodes; i++) {
            n := shape.nodes[i];
            if (n.verb == Close) {
                continue;
            }
            radius := 3;
            if (2*i < len e.mask && e.mask[2*i]) {
                radius = 5;
            }
            at = editbox(nodes,at,n.x-radius,n.y-radius,n.x+radius,n.y+radius);
            if (n.verb == Quadratic) {
                nodes[at++] = Node(Move,n.x,n.y,0,0);
                nodes[at++] = Node(Segment,n.cx,n.cy,0,0);
                radius = 3;
                if (2*i+1 < len e.mask && e.mask[2*i+1]) {
                    radius = 5;
                }
                at = editbox(nodes,at,n.cx-radius,n.cy-radius,n.cx+radius,n.cy+radius);
            }
        }
        if (e.mode == 5 && at+5 < MaxNodes) {
            l := e.origin.x;
            r := e.current.x;
            t := e.origin.y;
            b := e.current.y;
            if (l > r) {
                (l,r) = (r,l);
            }
            if (t > b) {
                (t,b) = (b,t);
            }
            at = editbox(nodes,at,l,t,r,b);
        }
    }
    return ref Shape(16r1687df, 1, 0, nodes[0:at], nil, 0);
}


# Initialize every selection slot explicitly; no allocator contents are selection.
# Return private storage to the owner; no I/O, callbacks or shared roots.
editmask(count: int): array of int
{
    mask := array[count] of int;
    for (i := 0; i < count; i++) {
        mask[i] = 0;
    }
    return mask;
}
