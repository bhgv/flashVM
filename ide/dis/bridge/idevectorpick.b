# Immutable contour queries and object/contour selection policy.


# Query one immutable path. Return squared nearest-contour distance and even/odd
# filled containment. Bounds prune whole shapes and curve hulls; only nearby
# curves or fill-ray candidates are flattened. Coordinates are logical page units.
# The caller owns selection policy; this routine does not select or mutate paths.
editquery(e: ref Edit, shape: ref Shape, x, y: int, radius: real): (real,int)
{
    best := real radius*real radius+1.0;
    (left,top,right,bottom) := pickbounds(e,shape);
    if (real x < real left-radius || real x > real right+radius || real y < real top-radius || real y > real bottom+radius) {
        return (best,0);
    }
    previous := Point(0,0);
    start := previous;
    crossings := 0;
    for (i := 0; i < len shape.nodes; i++) {
        n := shape.nodes[i];
        if (n.verb == Move) {
            previous = start = Point(n.x,n.y);
            continue;
        }
        end := Point(n.x,n.y);
        if (n.verb == Close) {
            end = start;
        }
        low := previous.y;
        high := end.y;
        if (low > high) {
            (low,high) = (high,low);
        }
        minx := previous.x;
        maxx := end.x;
        if (minx > maxx) {
            (minx,maxx) = (maxx,minx);
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
            if (cy < low) {
                low = cy;
            }
            if (cy > high) {
                high = cy;
            }
            if (cx < minx) {
                minx = cx;
            }
            if (cx > maxx) {
                maxx = cx;
            }
        }
        if (real y < real low-radius || real y > real high+radius || real x > real maxx+radius ||
            ((!shape.fill || y < low || y > high) && real x < real minx-radius)) {
            previous = end;
            continue;
        }
        begin := previous;
        steps := 1;
        if (n.verb == Quadratic || n.verb == Cubic) {
            steps = 64;
            if (maxx-minx < 24 && high-low < 24) {
                steps = 8;
            }
        }
        for (step := 1; step <= steps; step++) {
            next := end;
            if (steps > 1) {
                next = curvepoint(begin,n,step,steps);
            }
            dx := real (next.x-previous.x);
            dy := real (next.y-previous.y);
            length := dx*dx+dy*dy;
            along := 0.0;
            if (length > 0.0) {
                along = (real (x-previous.x)*dx+real (y-previous.y)*dy)/length;
                if (along < 0.0) {
                    along = 0.0;
                }
                if (along > 1.0) {
                    along = 1.0;
                }
            }
            ex := real (x-previous.x)-dx*along;
            ey := real (y-previous.y)-dy*along;
            distance := ex*ex+ey*ey;
            if (distance < best) {
                best = distance;
            }
            if ((previous.y > y) != (next.y > y)) {
                if (x < previous.x+(y-previous.y)*(next.x-previous.x)/(next.y-previous.y)) {
                    crossings++;
                }
            }
            previous = next;
        }
    }
    return (best,shape.fill && (crossings & 1));
}


# Select topmost filled containment first for Objects; otherwise globally nearest
# eligible contour, preserving topmost ties. Nodes retains an existing selected
# figure on a distant click so marquee editing does not choose an unrelated fill.
editpick(doc: ref Document, x, y: int): int
{
    found := -1;
    best := real doc.pickradius*real doc.pickradius+1.0;
    for (i := len doc.shapes-1; i >= 0; i--) {
        shape := doc.shapes[i];
        if (doc.tool == Select && shape.fill && len shape.area == GridBytes && covered(shape.area,x,y)) {
            return i;
        }
        (distance,inside) := editquery(doc.edit,doc.shapes[i],x,y,doc.pickradius);
        if (doc.tool == Select && inside) {
            return i;
        }
        if (distance <= real doc.pickradius*real doc.pickradius && distance < best) {
            best = distance;
            found = i;
        }
    }
    if (doc.tool == Nodes && found < 0) {
        return doc.edit.selected;
    }
    return found;
}


