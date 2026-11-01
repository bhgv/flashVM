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
        for (control := 0; control < 2; control++) {
            if (n.verb != Cubic && (control != 0 || n.verb != Quadratic)) {
                continue;
            }
            cx := n.cx;
            cy := n.cy;
            if (control) {
                cx = n.c2x;
                cy = n.c2y;
            }
            if (cx < left) {
                left = cx;
            }
            if (cy < top) {
                top = cy;
            }
            if (cx > right) {
                right = cx;
            }
            if (cy > bottom) {
                bottom = cy;
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


include "idevectorpick.b";


# Copy transformed geometry from the down snapshot, never from the previous move.
# Reject out-of-canvas results as a whole; preserve curve verbs and Undo baseline.
editmove(doc: ref Document, x, y: int): int
{
    e := doc.edit;
    shape := e.base[e.selected];
    dx := x-e.origin.x;
    dy := y-e.origin.y;
    if (dx == 0 && dy == 0) {
        doc.shapes = e.base;
        return Ok;
    }
    (left, top, right, bottom) := pickbounds(e,shape);
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
        for (control := 0; control < 3 && n.verb != Close; control++) {
            if (control && n.verb != Cubic && (control != 1 || n.verb != Quadratic)) {
                continue;
            }
            px := n.x;
            py := n.y;
            if (control) {
                (px, py) = (n.cx, n.cy);
                if (control == 2) {
                    (px,py) = (n.c2x,n.c2y);
                }
            }
            if (e.mode == 1 || (e.mode == 4 && e.mask[3*i+control])) {
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
            if (control == 2) {
                (n.c2x,n.c2y) = (px,py);
            } else if (control) {
                (n.cx, n.cy) = (px, py);
            }
            else {
                (n.x, n.y) = (px, py);
            }
        }
        old := shape.nodes[i];
        if (n.x != old.x || n.y != old.y || n.cx != old.cx || n.cy != old.cy || n.c2x != old.c2x || n.c2y != old.c2y) {
            changed = 1;
        }
        nodes[i] = n;
    }
    # A closed cubic stores the first anchor twice as path commands. Editing
    # either visible copy must keep the contour welded, including compound paths.
    if (e.mode == 4) {
        first := 0;
        for (i = 0; i < len nodes; i++) {
            old := shape.nodes[i];
            if (old.verb == Move) {
                first = i;
            }
            if (old.verb == Cubic && i+1 < len nodes && nodes[i+1].verb == Close &&
                old.x == shape.nodes[first].x && old.y == shape.nodes[first].y &&
                (e.mask[3*i] || e.mask[3*first])) {
                px := old.x+dx;
                py := old.y+dy;
                if (!pointok(px,py)) {
                    return Invalid;
                }
                nodes[first].x = nodes[i].x = px;
                nodes[first].y = nodes[i].y = py;
                changed = 1;
            }
        }
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
    if (doc.presentation != nil) {
        doc.presentation.dirty = 1;
    }
    if (doc.edit == nil) {
        doc.edit = ref Edit(nil, nil, -1, nil, nil, Point(0, 0), 0, 0, Point(0, 0));
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
            if (len e.mask != len shape.nodes*3) {
                e.mask = editmask(len shape.nodes*3);
            }
            if (doc.tool == Select) {
                (l, t, r, b) := pickbounds(e,shape);
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
                        hit = i*3;
                        best = distance;
                    }
                    for (control := 1; control <= 2; control++) {
                        if (n.verb != Cubic && (control != 1 || n.verb != Quadratic)) {
                            continue;
                        }
                        cx := n.cx;
                        cy := n.cy;
                        if (control == 2) {
                            (cx,cy) = (n.c2x,n.c2y);
                        }
                        distance = (x-cx)*(x-cx)+(y-cy)*(y-cy);
                        if (distance <= 64 && distance <= best) {
                            hit = i*3+control;
                            best = distance;
                        }
                    }
                }
                if (hit >= 0) {
                    if (!doc.control && !e.mask[hit]) {
                        e.mask = editmask(len shape.nodes*3);
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
            found := editpick(doc,x,y);
            if (doc.tool == Select) {
                e.selected = found;
                if (found >= 0) {
                    e.mode = 1;
                }
            } else if (found >= 0 && found != e.selected) {
                e.selected = found;
                e.mask = editmask(3*len doc.shapes[found].nodes);
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
                for (c := 0; c < 3 && n.verb != Close; c++) {
                    if (c && n.verb != Cubic && (c != 1 || n.verb != Quadratic)) {
                        continue;
                    }
                    px := n.x;
                    py := n.y;
                    if (c) {
                        (px, py) = (n.cx, n.cy);
                        if (c == 2) {
                            (px,py) = (n.c2x,n.c2y);
                        }
                    }
                    if ((px-e.origin.x)*(px-x) <= 0 && (py-e.origin.y)*(py-y) <= 0) {
                        e.mask[3*i+c] = 1;
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
    if (r < 0) {
        r = 0;
    }
    if (b < 0) {
        b = 0;
    }
    if (l > 1000) {
        l = 1000;
    }
    if (t > 600) {
        t = 600;
    }
    if (r > 1000) {
        r = 1000;
    }
    if (b > 600) {
        b = 600;
    }
    nodes[at:] = array[] of {Node(Move,l,t,0,0, 0, 0), Node(Segment,r,t,0,0, 0, 0),
        Node(Segment,r,b,0,0, 0, 0), Node(Segment,l,b,0,0, 0, 0), Node(Close,0,0,0,0, 0, 0)};
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
        (l,t,r,b) := pickbounds(e,shape);
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
            if (3*i < len e.mask && e.mask[3*i]) {
                radius = 5;
            }
            at = editbox(nodes,at,n.x-radius,n.y-radius,n.x+radius,n.y+radius);
            for (control := 1; control <= 2 && at+7 < MaxNodes; control++) {
                if (n.verb != Cubic && (control != 1 || n.verb != Quadratic)) {
                    continue;
                }
                cx := n.cx;
                cy := n.cy;
                anchor := Point(n.x,n.y);
                if (n.verb == Cubic && control == 1 && i > 0) {
                    anchor = Point(shape.nodes[i-1].x,shape.nodes[i-1].y);
                }
                if (control == 2) {
                    (cx,cy) = (n.c2x,n.c2y);
                }
                nodes[at++] = Node(Move,anchor.x,anchor.y,0,0,0,0);
                nodes[at++] = Node(Segment,cx,cy,0,0,0,0);
                radius = 3;
                if (3*i+control < len e.mask && e.mask[3*i+control]) {
                    radius = 5;
                }
                at = editbox(nodes,at,cx-radius,cy-radius,cx+radius,cy+radius);
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


# Reuse conservative bounds for the exact immutable selected/picked shape.
# The owner retains one shape reference only; a new geometry identity invalidates
# automatically, independently of revisions, Undo and worker publication.
pickbounds(e: ref Edit, shape: ref Shape): (int,int,int,int)
{
    if (e.boundshape != shape || len e.boundbox != 4) {
        (l,t,r,b) := editbounds(shape);
        e.boundbox = array[] of {l,t,r,b};
        e.boundshape = shape;
    }
    box := e.boundbox;
    return (box[0],box[1],box[2],box[3]);
}
