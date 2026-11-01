# Private implementation include for Idevector, not a separately loadable module.
# Published shape arrays/nodes are immutable. Cache state belongs to one Document.


# Serialize one immutable shape into a preflight-sized private byte array.
# bytes/at name caller-owned writable storage; shape is borrowed. Return the
# first unused offset. No allocation, callbacks, module load or shared mutation.
wireshape(bytes: array of byte, at: int, shape: ref Shape): int
{
    putword(bytes, at, shape.color);
    putword(bytes, at + 8, shape.width);
    putword(bytes, at + 16, shape.fill);
    putword(bytes, at + 24, len shape.nodes);
    at += 32;
    for (part := 0; part < len shape.nodes; part++) {
        node := shape.nodes[part];
        putword(bytes, at, node.verb);
        putword(bytes, at + 8, node.x);
        putword(bytes, at + 16, node.y);
        putword(bytes, at + 24, node.cx);
        putword(bytes, at + 32, node.cy);
        at += 40;
    }
    return at;
}


# Return unchanged geometry through a bulk byte copy instead of walking every
# node on Fill/Pen/color changes or Snapshot. doc is sole-owner mutable state;
# cache invalidation uses immutable array identity, never revision guesses or
# hashes. Live draft bytes append separately and never poison committed cache.
# The 56-byte header/record wire contract remains byte-for-byte unchanged.
# Prepare only immutable committed bytes; the geometry worker calls this before
# handing a result back, so the UI owner never walks its new contour nodes.
preparewire(doc: ref Document)
{
    if (doc.wirebody == "" || doc.wired != doc.shapes) {
        total := 0;
        for (index := 0; index < len doc.shapes; index++) {
            total += len doc.shapes[index].nodes;
        }
        geometry := array[8 + 32 * len doc.shapes + 40 * total] of byte;
        putword(geometry, 0, len doc.shapes);
        at := 8;
        for (index = 0; index < len doc.shapes; index++) {
            at = wireshape(geometry, at, doc.shapes[index]);
        }
        doc.wirebody = string geometry;
        doc.wired = doc.shapes;
        if (doc.wirebuilds < 2147483647) {
            doc.wirebuilds++;
        }
    }
}


# Borrow sole-owner doc and return a detached wire snapshot. Copy the cached
# committed body in bulk, serialize only live draft nodes, and preserve all
# existing header/record fields. No channel, callback or module load occurs.
wire(doc: ref Document): string
{
    preparewire(doc);
    extra: ref Shape;
    additional := 0;
    if (len doc.points != 0) {
        extra = draft(doc);
        additional = 32 + 40 * len extra.nodes;
    }
    bytes := array[48 + len doc.wirebody + additional] of byte;
    putword(bytes, 0, doc.tool);
    putword(bytes, 8, doc.color);
    putword(bytes, 16, doc.width);
    putword(bytes, 24, doc.fill);
    putword(bytes, 32, doc.nundo);
    putword(bytes, 40, doc.nredo);
    bytes[48:] = array of byte doc.wirebody;
    if (extra != nil) {
        putword(bytes, 48, len doc.shapes + 1);
        wireshape(bytes, 48 + len doc.wirebody, extra);
    }
    return string bytes;
}


# Write one uint32 in the existing eight-hex-digit scalar encoding.
# Caller owns the admitted bytes/range; no allocation or user work occurs here.
putword(bytes: array of byte, at, value: int)
{
    digits := "0123456789abcdef";
    for (digit := 0; digit < 8; digit++) {
        bytes[at + digit] = byte digits[(value >> (28 - digit * 4)) & 15];
    }
}
