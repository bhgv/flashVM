# Post-release cubic fitting on the private Limbo worker. Source samples remain
# immutable; failed fits subdivide rather than discarding corners or small loops.


# Evaluate a borrowed Q/C command from start at fraction part/steps. Integer
# logical output is shared by union/picking; real products avoid cubic overflow.
curvepoint(start: Point, node: Node, part, steps: int): Point
{
    t := real part / real steps;
    u := 1.0-t;
    if (node.verb == Cubic) {
        return Point(int (u*u*u*real start.x+3.0*u*u*t*real node.cx+
                          3.0*u*t*t*real node.c2x+t*t*t*real node.x+0.5),
                     int (u*u*u*real start.y+3.0*u*u*t*real node.cy+
                          3.0*u*t*t*real node.c2y+t*t*t*real node.y+0.5));
    }
    return Point(int (u*u*real start.x+2.0*u*t*real node.cx+t*t*real node.x+0.5),
                 int (u*u*real start.y+2.0*u*t*real node.cy+t*t*real node.y+0.5));
}


# Clamp a fitted handle to the representable page; the rounded/clamped curve is
# checked again against the source before acceptance. No mutation or I/O occurs.
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


# Fit one borrowed polyline interval by least squares with fixed endpoints.
# Chord parameters are precomputed by the caller. Return a private cubic value;
# singular short intervals use straight one-third/two-third controls.
brushcandidate(points: array of Point, parameters: array of real, first, last: int): Node
{
    a := points[first];
    b := points[last];
    aa := 0.0;
    ab := 0.0;
    bb := 0.0;
    ax := 0.0;
    ay := 0.0;
    bx := 0.0;
    by := 0.0;
    span := parameters[last]-parameters[first];
    for (i := first+1; i < last && span > 0.0; i++) {
        t := (parameters[i]-parameters[first])/span;
        u := 1.0-t;
        p := 3.0*u*u*t;
        q := 3.0*u*t*t;
        x := real points[i].x-u*u*u*real a.x-t*t*t*real b.x;
        y := real points[i].y-u*u*u*real a.y-t*t*t*real b.y;
        aa += p*p;
        ab += p*q;
        bb += q*q;
        ax += p*x;
        ay += p*y;
        bx += q*x;
        by += q*y;
    }
    cx := real (2*a.x+b.x)/3.0;
    cy := real (2*a.y+b.y)/3.0;
    c2x := real (a.x+2*b.x)/3.0;
    c2y := real (a.y+2*b.y)/3.0;
    determinant := aa*bb-ab*ab;
    if (determinant > 0.00000001) {
        cx = (ax*bb-bx*ab)/determinant;
        cy = (ay*bb-by*ab)/determinant;
        c2x = (bx*aa-ax*ab)/determinant;
        c2y = (by*aa-ay*ab)/determinant;
    }
    return Node(Cubic,b.x,b.y,brushclamp(int (cx+0.5),1000),
        brushclamp(int (cy+0.5),600),brushclamp(int (c2x+0.5),1000),
        brushclamp(int (c2y+0.5),600));
}


# Fit a closed borrowed contour globally, using complete compound-figure extents
# width/height for the per-axis 1/40 error budget. An explicit work stack bounds
# depth, never recurses, and splits at the worst offending original sample.
# Four initial spans prevent closed-loop collapse; every accepted rounded cubic
# is tested against all its input samples. Straight terminal edges retain detail.
# Return private M/C/L/Z commands, or nil for invalid input/capacity/load failure.
brushfit(points: array of Point, width, height: int): array of Node
{
    count := len points;
    if (count < 4 || count > MaxNodes || width < 1 || width > 1000 || height < 1 || height > 600 ||
        points[0].x != points[count-1].x || points[0].y != points[count-1].y) {
        return nil;
    }
    math := load Math Math->PATH;
    if (math == nil) {
        return nil;
    }
    parameters := array[count] of real;
    parameters[0] = 0.0;
    for (i := 0; i < count; i++) {
        if (!pointok(points[i].x,points[i].y)) {
            return nil;
        }
        if (i > 0) {
            dx := real (points[i].x-points[i-1].x);
            dy := real (points[i].y-points[i-1].y);
            parameters[i] = parameters[i-1]+math->sqrt(dx*dx+dy*dy);
        }
    }
    starts := array[MaxNodes] of int;
    ends := array[MaxNodes] of int;
    pending := 0;
    # Reverse push gives stable forward contour order with bounded DFS storage.
    for (quarter := 3; quarter >= 0; quarter--) {
        first := (count-1)*quarter/4;
        last := (count-1)*(quarter+1)/4;
        if (last > first) {
            starts[pending] = first;
            ends[pending++] = last;
        }
    }
    output := array[MaxNodes] of Node;
    output[0] = Node(Move,points[0].x,points[0].y,0,0,0,0);
    used := 1;
    while (pending > 0) {
        pending--;
        first := starts[pending];
        last := ends[pending];
        if (used+1 >= MaxNodes) {
            return nil;
        }
        if (last-first < 3) {
            for (i = first+1; i <= last; i++) {
                if (used+1 >= MaxNodes) {
                    return nil;
                }
                output[used++] = Node(Segment,points[i].x,points[i].y,0,0,0,0);
            }
            continue;
        }
        candidate := brushcandidate(points,parameters,first,last);
        worst := 0.0;
        split := (first+last)/2;
        span := parameters[last]-parameters[first];
        for (i = first+1; i < last && span > 0.0; i++) {
            t := (parameters[i]-parameters[first])/span;
            u := 1.0-t;
            dx := (u*u*u*real points[first].x+3.0*u*u*t*real candidate.cx+
                3.0*u*t*t*real candidate.c2x+t*t*t*real candidate.x-real points[i].x)*40.0/real width;
            dy := (u*u*u*real points[first].y+3.0*u*u*t*real candidate.cy+
                3.0*u*t*t*real candidate.c2y+t*t*t*real candidate.y-real points[i].y)*40.0/real height;
            error := dx*dx;
            if (dy*dy > error) {
                error = dy*dy;
            }
            if (error > worst) {
                worst = error;
                split = i;
            }
        }
        # Check the reverse direction too: a least-squares curve must not bow
        # away between source knots. Distances are normalized by each axis budget.
        # Sampling is bounded; this is a discrete fit check, not a continuous proof.
        for (sample := 1; sample < 64 && worst <= 1.0; sample++) {
            t := real sample/64.0;
            u := 1.0-t;
            px := (u*u*u*real points[first].x+3.0*u*u*t*real candidate.cx+
                3.0*u*t*t*real candidate.c2x+t*t*t*real candidate.x)*40.0/real width;
            py := (u*u*u*real points[first].y+3.0*u*u*t*real candidate.cy+
                3.0*u*t*t*real candidate.c2y+t*t*t*real candidate.y)*40.0/real height;
            nearest := 1.0e30;
            for (edge := first; edge < last; edge++) {
                ax := real points[edge].x*40.0/real width;
                ay := real points[edge].y*40.0/real height;
                dx := real (points[edge+1].x-points[edge].x)*40.0/real width;
                dy := real (points[edge+1].y-points[edge].y)*40.0/real height;
                length := dx*dx+dy*dy;
                along := 0.0;
                if (length > 0.0) {
                    along = ((px-ax)*dx+(py-ay)*dy)/length;
                    if (along < 0.0) {
                        along = 0.0;
                    }
                    if (along > 1.0) {
                        along = 1.0;
                    }
                }
                ex := px-ax-along*dx;
                ey := py-ay-along*dy;
                error := ex*ex+ey*ey;
                if (error < nearest) {
                    nearest = error;
                }
            }
            if (nearest > 1.0) {
                worst = nearest;
                split = (first+last)/2;
            }
        }
        if (worst <= 1.0 && span > 0.0) {
            output[used++] = candidate;
        } else {
            # Avoid quadratic work on adversarial contours whose worst sample
            # repeatedly lies next to an endpoint. Balanced splitting retains
            # every input interval and bounds aggregate work to logarithmic depth.
            if (split-first < (last-first)/4 || last-split < (last-first)/4) {
                split = (first+last)/2;
            }
            starts[pending] = split;
            ends[pending++] = last;
            starts[pending] = first;
            ends[pending++] = split;
        }
    }
    output[used++] = Node(Close,0,0,0,0,0,0);
    return output[0:used];
}
