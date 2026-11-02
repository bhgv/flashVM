# Transactional node operations. This module contains no toolbar or transport code.
include "idevectornodecurve.b";
include "idevectornodebreak.b";


# Apply a command to selected anchors. Invalid selection/topology is refused
# without changing geometry, history or revision. New arrays are immutable once
# published; one accepted operation creates exactly one Undo record.
nodeoperation(doc: ref Document, operation: int): int
{
    e := doc.edit;
    if (doc.tool != Nodes || e == nil || e.selected < 0 ||
        e.selected >= len doc.shapes || len doc.points || e.mode ||
        doc.revision == 2147483647 || operation < AddNode || operation > SymmetricNode) {
        return Conflict;
    }
    shape := doc.shapes[e.selected];
    count := len shape.nodes;
    if (len e.mask != 3*count) {
        return Invalid;
    }
    chosen := array[count] of int;
    selected := 0;
    for (i := 0; i < count; i++) {
        chosen[i] = e.mask[3*i] && shape.nodes[i].verb != Close;
        selected += chosen[i];
    }
    # Closed cubic seams have one logical anchor represented by two commands.
    first := 0;
    for (i = 0; i < count; i++) {
        if (shape.nodes[i].verb == Move) {
            first = i;
        }
        if (i > first && i+1 < count && shape.nodes[i+1].verb == Close &&
            shape.nodes[i].x == shape.nodes[first].x && shape.nodes[i].y == shape.nodes[first].y &&
            (chosen[i] || chosen[first])) {
            chosen[first] = 1;
            chosen[i] = operation == DeleteNode || operation == StraightSpan || operation == BezierSpan;
        }
    }
    selected = 0;
    for (i = 0; i < count; i++) {
        selected += chosen[i];
    }
    if (!selected) {
        return Invalid;
    }
    nodes := array[MaxNodes] of Node;
    joins := array[MaxNodes] of {* => 0};
    mask := editmask(3*MaxNodes);
    used := 0;
    changed := 0;
    if (operation == BreakNode) {
        return nodebreakclosed(doc,chosen,selected);
    }
    if (operation == JoinNodes) {
        return nodejoin(doc,chosen,selected);
    }
    for (i = 0; i < count; i++) {
        n := shape.nodes[i];
        mode := 0;
        if (len shape.joins == count) {
            mode = shape.joins[i];
        }
        if (operation == DeleteNode && chosen[i]) {
            if (n.verb == Move) {
                next := i+1;
                while (next < count && shape.nodes[next].verb != Close &&
                       shape.nodes[next].verb != Move && chosen[next]) {
                    next++;
                }
                if (next >= count || shape.nodes[next].verb == Close || shape.nodes[next].verb == Move) {
                    changed = 1;
                    i = next;
                    if (next < count && shape.nodes[next].verb == Move) {
                        i--;
                    }
                    continue;
                }
                i = next;
                n = shape.nodes[i];
                n = Node(Move,n.x,n.y,0,0,0,0);
                if (len shape.joins == count) {
                    mode = shape.joins[i];
                }
            } else {
                changed = 1;
                continue;
            }
            changed = 1;
        }
        if ((operation == StraightSpan || operation == BezierSpan) && i > 0 &&
            chosen[i] && chosen[i-1] && n.verb != Move && n.verb != Close) {
            if (operation == StraightSpan) {
                n = Node(Segment,n.x,n.y,0,0,0,0);
            } else {
                n = nodecubic(n,Point(shape.nodes[i-1].x,shape.nodes[i-1].y));
            }
            changed = 1;
        }
        if (operation >= SmoothNode && i > 0 && n.verb != Move && n.verb != Close &&
            (chosen[i] || chosen[i-1])) {
            n = nodecubic(n,Point(shape.nodes[i-1].x,shape.nodes[i-1].y));
        }
        if (chosen[i] && operation >= CuspNode) {
            mode = operation-CuspNode;
            changed = 1;
        }
        if (used >= MaxNodes) {
            return Limit;
        }
        if (operation == AddNode && i > 0 && chosen[i-1] &&
            n.verb != Move && n.verb != Close) {
            if (used+1 >= MaxNodes) {
                return Limit;
            }
            (a,b) := nodesplit(n,Point(shape.nodes[i-1].x,shape.nodes[i-1].y));
            nodes[used] = a;
            mask[3*used] = 1;
            used++;
            n = b;
            changed = 1;
        }
        nodes[used] = n;
        joins[used] = mode;
        mask[3*used] = chosen[i];
        used++;

    }
    if (changed && operation == DeleteNode && used < 2) {
        return removeselectedfigure(doc);
    }
    if (!changed || used < 2 || used > MaxNodes) {
        return Invalid;
    }
    # Establish requested tangent alignment immediately, then retain its mode.
    if (operation == SmoothNode || operation == SymmetricNode) {
        moved := editmask(3*used);
        for (i = 0; i < used; i++) {
            if (!mask[3*i]) {
                continue;
            }
            (incoming,outgoing) := nodearms(nodes[0:used],i);
            if (incoming >= 0 && outgoing >= 0) {
                moved[3*outgoing+1] = 1;
            }
        }
        proxy := ref Shape(shape.color,shape.width,shape.fill,nodes[0:used],nil,0,joins[0:used]);
        if (nodeconstraints(proxy,nodes[0:used],moved) != Ok) {
            return Invalid;
        }
    }
    return nodepublish(doc,nodes[0:used],joins[0:used],mask[0:3*used]);
}


# Publish an already checked private node transaction with a single history unit.
# The caller transfers arrays; no asynchronous worker may mutate them afterwards.
nodepublish(doc: ref Document, nodes: array of Node, joins, mask: array of int): int
{
    e := doc.edit;
    old := doc.shapes;
    total := len nodes;
    for (part := 0; part < len old; part++) {
        if (part != e.selected) {
            total += len old[part].nodes;
        }
    }
    if (len nodes > MaxNodes || total > MaxTotal) {
        return Limit;
    }
    shape := old[e.selected];
    for (i := 0; i < len nodes; i++) {
        n := nodes[i];
        if (!pointok(n.x,n.y) || !pointok(n.cx,n.cy) || !pointok(n.c2x,n.c2y)) {
            return Invalid;
        }
    }
    result := array[len old] of ref Shape;
    result[0:] = old;
    result[e.selected] = ref Shape(shape.color,shape.width,shape.fill,nodes,nil,0,joins);
    doc.nundo = push(doc.undo,doc.nundo,old);
    doc.nredo = 0;
    doc.revision++;
    doc.shapes = result;
    e.mask = mask;
    e.boundshape = nil;
    if (doc.presentation != nil) {
        doc.presentation.dirty = 1;
    }
    return Ok;
}


# Weld exactly two adjacent anchors to their midpoint. Two open subpaths may
# also be joined when the first ends immediately before the second Move.
# Nonadjacent interior anchors are refused instead of deleting an intervening arc.
nodejoin(doc: ref Document, chosen: array of int, selected: int): int
{
    if (selected != 2) {
        return Invalid;
    }
    a := -1;
    b := -1;
    shape := doc.shapes[doc.edit.selected];
    for (i := 0; i < len chosen; i++) {
        if (chosen[i]) {
            if (a < 0) {
                a = i;
            } else {
                b = i;
            }
        }
    }
    if (shape.nodes[a].verb == Move && b+1 == len shape.nodes &&
        shape.nodes[b].verb != Close) {
        nodes := array[len shape.nodes+1] of Node;
        if (len nodes > MaxNodes) {
            return Limit;
        }
        nodes[0:] = shape.nodes;
        x := (nodes[a].x+nodes[b].x)/2;
        y := (nodes[a].y+nodes[b].y)/2;
        nodes[a].x = nodes[b].x = x;
        nodes[a].y = nodes[b].y = y;
        nodes[len nodes-1] = Node(Close,0,0,0,0,0,0);
        joins := array[len nodes] of {* => 0};
        if (len shape.joins == len shape.nodes) {
            joins[0:] = shape.joins;
        }
        mask := editmask(3*len nodes);
        mask[3*a] = 1;
        return nodepublish(doc,nodes,joins,mask);
    }
    if (b != a+1 || shape.nodes[a].verb == Close) {
        return Invalid;
    }
    nodes := array[len shape.nodes-1] of Node;
    joins := array[len nodes] of {* => 0};
    mask := editmask(3*len nodes);
    x := (shape.nodes[a].x+shape.nodes[b].x)/2;
    y := (shape.nodes[a].y+shape.nodes[b].y)/2;
    at := 0;
    for (i = 0; i < len shape.nodes; i++) {
        if (i == b) {
            continue;
        }
        nodes[at] = shape.nodes[i];
        if (len shape.joins == len shape.nodes) {
            joins[at] = shape.joins[i];
        }
        if (i == a) {
            nodes[at].x = x;
            nodes[at].y = y;
            mask[3*at] = 1;
        }
        at++;
    }
    return nodepublish(doc,nodes,joins,mask);
}


include "idevectorclipboard.b";
