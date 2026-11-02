# Cutting paths preserves their exact segments, including closed cubic spans.


# Cut all selected interior anchors. Closed contours rotate traversal to the
# first cut and lose Close; other cuts get a duplicate Move. Open end anchors
# cannot be split and are refused atomically. Result arrays belong to this call.
nodebreakclosed(doc: ref Document, chosen: array of int, selected: int): int
{
    shape := doc.shapes[doc.edit.selected];
    capacity := len shape.nodes+2*selected;
    nodes := array[capacity] of Node;
    joins := array[capacity] of {* => 0};
    mask := editmask(3*capacity);
    at := 0;
    first := 0;
    while (first < len shape.nodes) {
        last := first+1;
        while (last < len shape.nodes && shape.nodes[last].verb != Move && shape.nodes[last].verb != Close) {
            last++;
        }
        closed := last < len shape.nodes && shape.nodes[last].verb == Close;
        cut := -1;
        for (i := first; i < last; i++) {
            if (chosen[i]) {
                cut = i;
                break;
            }
        }
        if (cut < 0) {
            end := last+closed;
            for (i = first; i < end; i++) {
                nodes[at] = shape.nodes[i];
                if (len shape.joins == len shape.nodes) {
                    joins[at] = shape.joins[i];
                }
                at++;
            }
        } else if (!closed) {
            if (chosen[first] || chosen[last-1]) {
                return Invalid;
            }
            for (i = first; i < last; i++) {
                n := shape.nodes[i];
                nodes[at] = n;
                if (len shape.joins == len shape.nodes) {
                    joins[at] = shape.joins[i];
                }
                mask[3*at] = chosen[i];
                at++;
                if (chosen[i]) {
                    nodes[at] = Node(Move,n.x,n.y,0,0,0,0);
                    mask[3*at] = 1;
                    at++;
                }
            }
        } else {
            duplicate := shape.nodes[last-1].x == shape.nodes[first].x &&
                         shape.nodes[last-1].y == shape.nodes[first].y;
            # Canonicalize a duplicate first/last anchor for one closed seam.
            selectedfirst := chosen[first] || (duplicate && chosen[last-1]);
            if (selectedfirst) {
                cut = first;
            }
            n := shape.nodes[cut];
            nodes[at] = Node(Move,n.x,n.y,0,0,0,0);
            mask[3*at] = 1;
            at++;
            for (step := 1; step <= last-first; step++) {
                i = cut+step;
                implicit := 0;
                if (i >= last) {
                    i = first+(i-last);
                }
                if (i == first) {
                    if (duplicate) {
                        continue;
                    }
                    implicit = 1;
                }
                n = shape.nodes[i];
                if (implicit) {
                    n = Node(Segment,n.x,n.y,0,0,0,0);
                }
                nodes[at] = n;
                if (len shape.joins == len shape.nodes) {
                    joins[at] = shape.joins[i];
                }
                selectedhere := chosen[i];
                if (duplicate && i == last-1) {
                    selectedhere = selectedfirst;
                }
                mask[3*at] = selectedhere;
                at++;
                if (i != cut && !(duplicate && cut == first && i == last-1) && selectedhere) {
                    nodes[at] = Node(Move,n.x,n.y,0,0,0,0);
                    mask[3*at] = 1;
                    at++;
                }
            }
            mask[3*(at-1)] = 1;
        }
        first = last+closed;
    }
    if (at > MaxNodes) {
        return Limit;
    }
    return nodepublish(doc,nodes[0:at],joins[0:at],mask[0:3*at]);
}
