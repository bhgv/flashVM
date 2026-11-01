# Independent geometric tolerance oracle; never calls the implementation evaluator.


# Intersect a flattened segment with the per-axis error box around one source point.
# Borrow four endpoint coordinates and the point/budgets; return whether some point
# on the segment satisfies both axes. This is a test slab calculation, not fitting.
fitbox(ax, ay, bx, by, px, py, tx, ty: real): int
{
    low := 0.0;
    high := 1.0;
    for (axis := 0; axis < 2; axis++) {
        origin := ax;
        delta := bx-ax;
        point := px;
        tolerance := tx;
        if (axis) {
            origin = ay;
            delta = by-ay;
            point = py;
            tolerance = ty;
        }
        if (delta == 0.0) {
            if (origin < point-tolerance || origin > point+tolerance) {
                return 0;
            }
        } else {
            first := (point-tolerance-origin)/delta;
            last := (point+tolerance-origin)/delta;
            if (first > last) {
                (first,last) = (last,first);
            }
            if (first > low) {
                low = first;
            }
            if (last < high) {
                high = last;
            }
        }
    }
    return low <= high;
}


# A wavy oval requires finer curves than the old1/40 budget. Every input point
# must meet width/100 and height/100 independently against flattened output.
# The0.15 logical-unit allowance covers this independent32-step test flattening.
fitaccuracy()
{
    math := load Math Math->PATH;
    require(math != nil,"tolerance math");
    points := array[129] of Idevector->Point;
    l := 1000;
    r := 0;
    top := 600;
    bottom := 0;
    for (i := 0; i < len points-1; i++) {
        angle := real i*6.283185307179586/128.0;
        wave := math->sin(angle*7.0);
        points[i] = Idevector->Point(int (500.5+(300.0+12.0*wave)*math->cos(angle)),
            int (300.5+(180.0+8.0*wave)*math->sin(angle)));
        if (points[i].x < l) {
            l = points[i].x;
        }
        if (points[i].x > r) {
            r = points[i].x;
        }
        if (points[i].y < top) {
            top = points[i].y;
        }
        if (points[i].y > bottom) {
            bottom = points[i].y;
        }
    }
    points[128] = points[0];
    nodes := vector->brushfit(points,r-l,bottom-top);
    require(len nodes >= 6 && len nodes < 70,"wavy contour remains a bounded fitted path");
    tx := real (r-l)/100.0+0.15;
    ty := real (bottom-top)/100.0+0.15;
    for (i = 0; i < len points; i++) {
        hit := 0;
        start := nodes[0];
        for (n := 1; n < len nodes && !hit; n++) {
            node := nodes[n];
            if (node.verb == vector->Close) {
                continue;
            }
            ax := real start.x;
            ay := real start.y;
            steps := 1;
            if (node.verb == vector->Cubic) {
                steps = 32;
            }
            for (part := 1; part <= steps && !hit; part++) {
                t := real part/real steps;
                u := 1.0-t;
                bx := real node.x;
                by := real node.y;
                if (steps > 1) {
                    bx = u*u*u*real start.x+3.0*u*u*t*real node.cx+
                        3.0*u*t*t*real node.c2x+t*t*t*real node.x;
                    by = u*u*u*real start.y+3.0*u*u*t*real node.cy+
                        3.0*u*t*t*real node.c2y+t*t*t*real node.y;
                }
                hit = fitbox(ax,ay,bx,by,real points[i].x,real points[i].y,tx,ty);
                ax = bx;
                ay = by;
            }
            start = node;
        }
        require(hit,"original contour sample satisfies1/100 on each axis");
    }
}
