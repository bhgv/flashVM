# Cubic helpers for node transactions; all arrays are private owner values.


# Convert one explicit segment to a cubic without changing its curve.
# start is the previous endpoint; Close/Move are returned unchanged.
nodecubic(n: Node, start: Point): Node
{
    if (n.verb == Segment) {
        n.cx = (2*start.x+n.x)/3;
        n.cy = (2*start.y+n.y)/3;
        n.c2x = (start.x+2*n.x)/3;
        n.c2y = (start.y+2*n.y)/3;
        n.verb = Cubic;
    } else if (n.verb == Quadratic) {
        n.c2x = (n.x+2*n.cx)/3;
        n.c2y = (n.y+2*n.cy)/3;
        n.cx = (start.x+2*n.cx)/3;
        n.cy = (start.y+2*n.cy)/3;
        n.verb = Cubic;
    }
    return n;
}


# Split one line/Q/C at t=1/2 using de Casteljau; no resampling or fitting.
# Integer page coordinates round down; each new control stays in the convex hull.
nodesplit(n: Node, start: Point): (Node, Node)
{
    if (n.verb == Segment) {
        return (Node(Segment,(start.x+n.x)/2,(start.y+n.y)/2,0,0,0,0),n);
    }
    n = nodecubic(n,start);
    a := Point((start.x+n.cx)/2,(start.y+n.cy)/2);
    b := Point((n.cx+n.c2x)/2,(n.cy+n.c2y)/2);
    c := Point((n.c2x+n.x)/2,(n.c2y+n.y)/2);
    d := Point((a.x+b.x)/2,(a.y+b.y)/2);
    e := Point((b.x+c.x)/2,(b.y+c.y)/2);
    middle := Point((d.x+e.x)/2,(d.y+e.y)/2);
    return (Node(Cubic,middle.x,middle.y,a.x,a.y,d.x,d.y),
            Node(Cubic,n.x,n.y,e.x,e.y,c.x,c.y));
}


# Find the incoming/outgoing cubic controls for one anchor, including a closed
# contour's duplicated first endpoint. Return -1 for a missing side; no mutation.
nodearms(nodes: array of Node, anchor: int): (int, int)
{
    incoming := -1;
    outgoing := -1;
    if (nodes[anchor].verb == Cubic) {
        incoming = anchor;
    }
    if (anchor+1 < len nodes && nodes[anchor+1].verb == Cubic) {
        outgoing = anchor+1;
    }
    if (incoming >= 0 && outgoing >= 0) {
        return (incoming,outgoing);
    }
    if (nodes[anchor].verb != Move &&
        (anchor+1 >= len nodes || nodes[anchor+1].verb != Close)) {
        return (incoming,outgoing);
    }
    first := anchor;
    while (first > 0 && nodes[first].verb != Move) {
        first--;
    }
    last := first+1;
    while (last < len nodes && nodes[last].verb != Move && nodes[last].verb != Close) {
        last++;
    }
    if (last < len nodes && nodes[last].verb == Close && last > first+1 &&
        nodes[last-1].x == nodes[first].x && nodes[last-1].y == nodes[first].y) {
        if (anchor == first && nodes[last-1].verb == Cubic) {
            incoming = last-1;
        }
        if (anchor == last-1 && first+1 < last && nodes[first+1].verb == Cubic) {
            outgoing = first+1;
        }
    }
    return (incoming,outgoing);
}


# Align opposite controls for immutable shape joins after one selected handle
# moves. Smooth preserves the opposite baseline length; symmetric copies length.
# Refuse the whole movement if any reflected control leaves the bounded page.
nodeconstraints(shape: ref Shape, nodes: array of Node, mask: array of int): int
{
    if (len shape.joins != len nodes) {
        return Ok;
    }
    math := load Math Math->PATH;
    if (math == nil) {
        return Invalid;
    }
    for (i := 0; i < len nodes; i++) {
        mode := shape.joins[i];
        if (!mode || nodes[i].verb == Close) {
            continue;
        }
        (incoming,outgoing) := nodearms(nodes,i);
        if (incoming < 0 || outgoing < 0) {
            continue;
        }
        left := mask[3*incoming+2];
        right := mask[3*outgoing+1];
        if (left == right || mask[3*i]) {
            continue;
        }
        anchor := Point(nodes[i].x,nodes[i].y);
        moving := Point(nodes[incoming].c2x,nodes[incoming].c2y);
        opposite := Point(shape.nodes[outgoing].cx,shape.nodes[outgoing].cy);
        if (right) {
            moving = Point(nodes[outgoing].cx,nodes[outgoing].cy);
            opposite = Point(shape.nodes[incoming].c2x,shape.nodes[incoming].c2y);
        }
        dx := real (moving.x-anchor.x);
        dy := real (moving.y-anchor.y);
        length := math->sqrt(dx*dx+dy*dy);
        ratio := 1.0;
        if (mode == 1 && length > 0.0) {
            ox := real (opposite.x-shape.nodes[i].x);
            oy := real (opposite.y-shape.nodes[i].y);
            ratio = math->sqrt(ox*ox+oy*oy)/length;
        }
        if (length == 0.0 && mode == 1) {
            continue;
        }
        x := int (real anchor.x-dx*ratio);
        y := int (real anchor.y-dy*ratio);
        if (!pointok(x,y)) {
            return Invalid;
        }
        if (right) {
            (nodes[incoming].c2x,nodes[incoming].c2y) = (x,y);
        } else {
            (nodes[outgoing].cx,nodes[outgoing].cy) = (x,y);
        }
    }
    return Ok;
}
