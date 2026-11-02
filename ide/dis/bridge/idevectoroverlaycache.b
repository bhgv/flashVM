# Reuse immutable node decorations when only gesture admission metadata changes.


# Compare copied selection, geometry identity and gesture kind. A cache hit
# rewrites six small plan words and reuses its immutable mask/binding tail.
# The caller owns doc/p; no alias can mutate p.selection, no channel is touched.
# Return0 for a genuine geometry/selection/marquee change requiring a rebuild.
presentationreuse(doc: ref Document, p: ref Presentation): int
{
    e := doc.edit;
    if (doc.tool != Nodes || p.tool != Nodes || doc.shapes != p.base ||
        e == nil || e.selected != p.selected || e.selected < 0 ||
        (e.mode != 0 && e.mode != 4) || (p.mode != 0 && p.mode != 4) ||
        len e.mask != len p.selection || len p.plan < 48) {
        return 0;
    }
    for (i := 0; i < len e.mask; i++) {
        if (e.mask[i] != p.selection[i]) {
            return 0;
        }
    }
    header := array[48] of byte;
    putword(header,0,e.selected);
    putword(header,8,doc.revision);
    putword(header,16,e.applied.x);
    putword(header,24,e.applied.y);
    putword(header,32,len doc.shapes[e.selected].nodes);
    putword(header,40,len e.bindings);
    p.plan = string header+p.plan[48:];
    p.mode = e.mode;
    p.dirty = 0;
    return 1;
}
