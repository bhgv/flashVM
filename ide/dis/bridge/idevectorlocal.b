# Typed owner-produced bindings for frame-free local FlashVM node motion.


# Bind four square corners to one source point, retaining unclipped radius/signs.
# bytes are owner-local binding slots; point is one-based3*node+coordinate+1.
# Native modifiers clamp the derived corners, so page-edge handles retain size.
editbindbox(bindings: array of int, at, point, radius: int)
{
    for (corner := 0; corner < 4; corner++) {
        bindings[at+corner] = (point << 8) | (radius << 3) | (corner+1);
    }
    bindings[at+4] = 0;
}


# Emit only an admitted Nodes translation plan: selected source, revision,
# current baseline pointer, source masks and overlay bindings. No native pointer,
# heap address, document authority or filename crosses this private value stream.
# Return empty outside a gesture; the native owner then uses ordinary input routing.
editplan(doc: ref Document): string
{
    e := doc.edit;
    if (doc.tool != Nodes || e == nil || e.mode != 4 || e.selected < 0 ||
        e.selected >= len doc.shapes || len doc.points == 0) {
        return "";
    }
    count := len doc.shapes[e.selected].nodes;
    if (len e.mask != 3*count || len e.bindings < count) {
        return "";
    }
    bytes := array[48+8*count+8*len e.bindings] of byte;
    putword(bytes,0,e.selected);
    putword(bytes,8,doc.revision);
    putword(bytes,16,e.applied.x);
    putword(bytes,24,e.applied.y);
    putword(bytes,32,count);
    putword(bytes,40,len e.bindings);
    at := 48;
    for (i := 0; i < count; i++) {
        putword(bytes,at,e.mask[3*i] | (e.mask[3*i+1] << 1) | (e.mask[3*i+2] << 2));
        at += 8;
    }
    for (i = 0; i < len e.bindings; i++) {
        putword(bytes,at,e.bindings[i]);
        at += 8;
    }
    return string bytes;
}
