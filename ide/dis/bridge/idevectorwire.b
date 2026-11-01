# Private implementation include for Idevector, not a separately loadable module.
# Published shape arrays/nodes are immutable. Cache state belongs to one Document.


# Serialize one immutable shape into a preflight-sized private byte array.
# bytes/at name caller-owned writable storage; shape is borrowed. Return the
# first unused offset. No allocation, callbacks, module load or shared mutation.
wireshape(bytes: array of byte, at: int, shape: ref Shape, cubic: int, work: ref Work): int
{
    putword(bytes, at, shape.color);
    putword(bytes, at + 8, shape.width);
    putword(bytes, at + 16, shape.fill);
    putword(bytes, at + 24, len shape.nodes);
    at += 32;
    for (part := 0; part < len shape.nodes; part++) {
        if (part % 128 == 0) {
            checkwork(work);
        }
        node := shape.nodes[part];
        putword(bytes, at, node.verb);
        putword(bytes, at + 8, node.x);
        putword(bytes, at + 16, node.y);
        putword(bytes, at + 24, node.cx);
        putword(bytes, at + 32, node.cy);
        if (cubic) {
            putword(bytes, at+40, node.c2x);
            putword(bytes, at+48, node.c2y);
        }
        at += 40+16*cubic;
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
    cubic := doc.wirecubic || doc.tool == Ellipse;
    if (doc.wired != doc.shapes) {
        for (index := 0; index < len doc.shapes && !cubic; index++) {
            nodes := doc.shapes[index].nodes;
            for (part := 0; part < len nodes; part++) {
                if (nodes[part].verb == Cubic) {
                    cubic = 1;
                    break;
                }
            }
        }
    }
    if (cubic != doc.wirecubic) {
        doc.wirecubic = cubic;
        doc.wirebody = "";
        doc.presentation = nil;
    }
    if (doc.wirebody == "" || doc.wired != doc.shapes) {
        total := 0;
        for (index := 0; index < len doc.shapes; index++) {
            total += len doc.shapes[index].nodes;
        }
        geometry := array[8 + 32 * len doc.shapes + (40+16*doc.wirecubic) * total] of byte;
        putword(geometry, 0, len doc.shapes);
        at := 8;
        for (index = 0; index < len doc.shapes; index++) {
            at = wireshape(geometry, at, doc.shapes[index], doc.wirecubic, doc.work);
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
    if (doc.tool >= Select) {
        segments := wireparts(doc);
        joined := "";
        for (segment := 0; segment < len segments; segment++) {
            joined += segments[segment];
        }
        return joined;
    }
    if (!doc.nativecapture && len doc.chain != 0 && len doc.points != 0) {
        return chainwire(doc);
    }
    preparewire(doc);
    extra: ref Shape;
    additional := 0;
    if (len doc.points != 0 && !doc.nativecapture) {
        extra = draft(doc);
        additional = 32 + (40+16*doc.wirecubic) * len extra.nodes;
    }
    header := cameraheader(doc);
    prefix := len header;
    bytes := array[prefix + len doc.wirebody + additional] of byte;
    bytes[0:] = header;
    putword(bytes, 8, doc.color);
    putword(bytes, 16, doc.width);
    putword(bytes, 24, doc.fill | ((len doc.chain != 0) << 1));
    putword(bytes, 32, doc.nundo);
    putword(bytes, 40, doc.nredo);
    bytes[prefix:] = array of byte doc.wirebody;
    if (extra != nil) {
        putword(bytes, prefix, len doc.shapes + 1);
        wireshape(bytes, prefix + len doc.wirebody, extra, doc.wirecubic, doc.work);
    }
    return string bytes;
}


# Borrow sole-owner doc and return a new small segment array. Committed body
# bytes remain shared immutable strings, never copied into the UI owner's reply.
# Native live ink has no doc.points; the portable draft fallback preserves wire.
wireparts(doc: ref Document): array of string
{
    if (doc.tool >= Select) {
        preparewire(doc);
        presentation(doc);
        p := doc.presentation;
        return array[] of {string cameraheader(doc),p.count,p.paths,p.overlay};
    }
    if (len doc.points != 0 && !doc.nativecapture) {
        return array[] of {wire(doc)};
    }
    preparewire(doc);
    header := cameraheader(doc);
    return array[] of {string header, doc.wirebody};
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


# Publish camera scalars only when nondefault; legacy default streams stay exact.
# Flag 0x10000 is a private IDE header extension, not a SWF/Dis64 format change.
cameraheader(doc: ref Document): array of byte
{
    extra := doc.zoom != 100 || doc.center.x != 500 || doc.center.y != 300;
    header := array[48+24*extra] of byte;
    putword(header,0,doc.tool | (extra << 16) | (doc.wirecubic << 17));
    putword(header,8,doc.color);
    putword(header,16,doc.width);
    putword(header,24,doc.fill | ((len doc.chain != 0) << 1));
    putword(header,32,doc.nundo);
    putword(header,40,doc.nredo);
    if (extra) {
        putword(header,48,doc.zoom);
        putword(header,56,doc.center.x);
        putword(header,64,doc.center.y);
    }
    return header;
}

include "idevectorpresentation.b";
