# Post-release brush contour fitting; executed only in the private Limbo worker.


# Compare both original coordinate distances strictly against figure extent/40.
# Zero-width axes cannot qualify. Integer products preserve subpixel thresholds.
brushnear(a, b: Point, width, height: int): int
{
    dx := a.x-b.x;
    dy := a.y-b.y;
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    return 40*dx < width && 40*dy < height;
}


# Keep a derived handle inside the bounded canvas. Pure scalar conversion.
brushclamp(value, maximum: int): int
{
    if (value < 0) {
        return 0;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}


# Replace a bounded dense run by a two-piece interpolating quadratic spline.
# Endpoints and its middle authored knot are interpolated exactly; the two
# controls give a common tangent at that knot. Other nearby samples are fitted,
# not promised exact interpolation. Existing node editing exposes both controls.
# Borrow closed points; return a private immutable M/Q/L/Z path. No I/O/UI occurs.
brushfit(points: array of Point, width, height: int): array of Node
{
    if (len points < 4 || len points > MaxNodes || width < 1 || width > 1000 || height < 1 || height > 600 ||
        points[0].x != points[len points-1].x || points[0].y != points[len points-1].y) {
        return nil;
    }
    for (index := 0; index < len points; index++) {
        if (!pointok(points[index].x,points[index].y)) {
            return nil;
        }
    }
    i := 0;
    nodes := array[len points+1] of Node;
    used := 1;
    nodes[0] = Node(Move,points[0].x,points[0].y,0,0);
    i = 0;
    while (i < len points-2) {
        last := i+1;
        # Bound run span as well as neighbour spacing: a dense closed contour
        # must never collapse into one spline merely because every edge is short.
        while (last+1 < len points-1 && brushnear(points[last-1],points[last],width,height) &&
               brushnear(points[last],points[last+1],width,height) &&
               brushnear(points[i],points[last+1],4*width,4*height)) {
            last++;
        }
        if (last-i >= 3) {
            a := points[i];
            middle := points[(i+last)/2];
            end := points[last];
            hx := (end.x-a.x)/4;
            hy := (end.y-a.y)/4;
            nodes[used++] = Node(Quadratic,middle.x,middle.y,
                brushclamp(middle.x-hx,1000),brushclamp(middle.y-hy,600));
            nodes[used++] = Node(Quadratic,end.x,end.y,
                brushclamp(middle.x+hx,1000),brushclamp(middle.y+hy,600));
            i = last;
        } else {
            i++;
            nodes[used++] = Node(Segment,points[i].x,points[i].y,0,0);
        }
    }
    nodes[used++] = Node(Close,0,0,0,0);
    return nodes[0:used];
}
