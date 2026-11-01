# Canvas camera policy belongs to the Limbo document owner, not native rendering.


# Update a bounded camera without changing source/history. step0 resets; positive
# and negative steps zoom around a copied logical focus while retaining that focus.
viewport(doc: ref Document, step, x, y: int)
{
    if (step == 0) {
        doc.zoom = 100;
        doc.center = Point(500,300);
        return;
    }
    old := doc.zoom;
    next := old*5/4;
    if (step < 0) {
        next = old*4/5;
    }
    if (next < 25) {
        next = 25;
    }
    if (next > 1600) {
        next = 1600;
    }
    doc.center = Point(brushclamp(x+(doc.center.x-x)*old/next,1000),
                       brushclamp(y+(doc.center.y-y)*old/next,600));
    doc.zoom = next;
}


# Own a rubber-band gesture in logical page coordinates. Commit camera only on
# release; it never creates an SVG shape, a source revision or an Undo item.
zoompointer(doc: ref Document, phase, x, y: int): int
{
    if (doc.presentation != nil) {
        doc.presentation.dirty = 1;
    }
    if (phase == 1) {
        if (len doc.points != 0) {
            return Conflict;
        }
        doc.points = array[] of {Point(x,y),Point(x,y)};
        return Ok;
    }
    if (len doc.points != 2) {
        return Conflict;
    }
    doc.points[1] = Point(x,y);
    if (phase == 3) {
        a := doc.points[0];
        dx := x-a.x;
        dy := y-a.y;
        if (dx < 0) {
            dx = -dx;
        }
        if (dy < 0) {
            dy = -dy;
        }
        if (dx >= 4 && dy >= 4) {
            zoom := 100000/dx;
            if (60000/dy < zoom) {
                zoom = 60000/dy;
            }
            if (zoom > 1600) {
                zoom = 1600;
            }
            if (zoom < 25) {
                zoom = 25;
            }
            doc.zoom = zoom;
            doc.center = Point((a.x+x)/2,(a.y+y)/2);
        }
        doc.points = nil;
    }
    return Ok;
}
