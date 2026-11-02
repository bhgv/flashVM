# Immutable vector deletion and SVG interchange, independent of session storage.


# Export the selected complete figure/compound contour. The temporary document
# borrows immutable geometry only; no history, channel or native resource escapes.
copy(doc: ref Document): string
{
    e := doc.edit;
    if (e == nil || e.selected < 0 || e.selected >= len doc.shapes || len doc.points) {
        return "";
    }
    temporary := create();
    temporary.shapes = array[] of {doc.shapes[e.selected]};
    return svg(temporary);
}


# Delete selected node endpoints, or the selected complete figure when no node
# endpoint is selected. Deleting every endpoint removes the figure. One accepted
# call is one Undo unit; active gestures/revision exhaustion are refused.
remove(doc: ref Document): int
{
    e := doc.edit;
    if (e == nil || e.selected < 0 || e.selected >= len doc.shapes ||
        len doc.points || e.mode || doc.revision == 2147483647) {
        return Conflict;
    }
    if (doc.tool == Nodes) {
        nodes := doc.shapes[e.selected].nodes;
        selected := 0;
        remaining := 0;
        controls := 0;
        for (i := 0; i < len nodes; i++) {
            if (nodes[i].verb != Close) {
                if (3*i < len e.mask && e.mask[3*i]) {
                    selected++;
                } else {
                    remaining++;
                }
            }
        }
        for (i = 0; i < len e.mask; i++) {
            if (i%3 && e.mask[i]) {
                controls++;
            }
        }
        if (!selected && controls) {
            return Conflict;
        }
        if (selected && remaining >= 2) {
            return nodeoperation(doc,DeleteNode);
        }
    }
    return removeselectedfigure(doc);
}


# Remove one already-admitted selected figure, including an exhausted node path.
# No recursive node interpretation: callers validated the revision/gesture first.
removeselectedfigure(doc: ref Document): int
{
    e := doc.edit;
    # Whole-figure removal also serves Cut, without modifying any copied shape.
    replacement := array[len doc.shapes-1] of ref Shape;
    at := 0;
    for (i := 0; i < len doc.shapes; i++) {
        if (i != e.selected) {
            replacement[at++] = doc.shapes[i];
        }
    }
    return vectorreplace(doc,replacement,-1);
}


# Parse before changing any owner state. Replace the selected figure at its
# existing z-order, otherwise append; retain all source geometry on any refusal.
paste(doc: ref Document, text: string): int
{
    if (len doc.points || doc.revision == 2147483647 || text == "") {
        return Conflict;
    }
    (status,inserted) := parse(text);
    if (status != Ok || len inserted.shapes == 0) {
        return Invalid;
    }
    selected := -1;
    if (doc.edit != nil && doc.edit.selected >= 0 && doc.edit.selected < len doc.shapes) {
        selected = doc.edit.selected;
    }
    count := len doc.shapes+len inserted.shapes-(selected >= 0);
    if (count > MaxShapes) {
        return Limit;
    }
    result := array[count] of ref Shape;
    at := 0;
    target := selected;
    if (target < 0) {
        target = len doc.shapes;
    }
    for (i := 0; i <= len doc.shapes; i++) {
        if (i == target) {
            for (part := 0; part < len inserted.shapes; part++) {
                result[at++] = inserted.shapes[part];
            }
        }
        if (i < len doc.shapes && i != selected) {
            result[at++] = doc.shapes[i];
        }
    }
    total := 0;
    for (i = 0; i < len result; i++) {
        total += len result[i].nodes;
    }
    if (total > MaxTotal) {
        return Limit;
    }
    return vectorreplace(doc,result,target);
}


# Commit one complete immutable shape array, sharing the ordinary model history.
# The caller has checked bounds; selected is the new selection or -1. No I/O.
vectorreplace(doc: ref Document, shapes: array of ref Shape, selected: int): int
{
    doc.nundo = push(doc.undo,doc.nundo,doc.shapes);
    doc.nredo = 0;
    doc.shapes = shapes;
    doc.revision++;
    if (doc.edit == nil && selected >= 0) {
        doc.edit = ref Edit(nil,nil,-1,nil,nil,Point(0,0),0,0,Point(0,0),nil,Point(0,0));
    }
    if (doc.edit != nil) {
        doc.edit.selected = selected;
        doc.edit.mask = nil;
        if (selected >= 0) {
            doc.edit.mask = editmask(3*len shapes[selected].nodes);
        }
        doc.edit.boundshape = nil;
        doc.edit.base = nil;
        doc.edit.mode = 0;
    }
    doc.presentation = nil;
    return Ok;
}
