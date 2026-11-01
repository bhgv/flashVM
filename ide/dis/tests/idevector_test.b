implement IdevectorTest;
include "sys.m";
include "idevector.m";
sys: Sys; # Test output and explicit isolated test-host failure control.
vector: Idevector; # Actual portable implementation under test.
checks: int; # Initialized active predicate count.


# Run the real model without native GUI or a mirror implementation.
IdevectorTest: module {
    init: fn();
};


# Keep every predicate active; fail text is also checked by the host driver.
require(ok: int, message: string)
{
    checks++;
    if (!ok) {
        sys->print("FAIL vector: %s\n", message);
        control := sys->open("/dev/sysctl", Sys->OWRITE);
        data := array of byte "halt";
        if (control != nil) {
            sys->write(control, data, len data);
        }
        raise "fail: vector model";
    }
}


# Style/header changes and a live draft must not reserialize committed nodes.
# Arrays are immutable; a new array identity must invalidate even at same revision.
wirecache()
{
    cached := vector->create();
    nodes := array[512] of Idevector->Node;
    nodes[0] = Idevector->Node(vector->Move, 10, 10, 0, 0);
    for (i := 1; i < len nodes; i++) {
        nodes[i] = Idevector->Node(vector->Segment, i, 20 + i % 100, 0, 0);
    }
    shape := ref Idevector->Shape(16r245ac4, 8, 0, nodes, nil, 0);
    cached.shapes = array[] of {shape};
    original := vector->wire(cached);
    require(cached.wirebuilds == 1, "initial geometry serialized once");
    changed: string;
    for (i = 0; i < 64; i++) {
        cached.fill = i % 2;
        cached.tool = vector->Brush;
        if (cached.fill) {
            cached.tool = vector->Pen;
        }
        changed = vector->wire(cached);
        pieces := vector->wireparts(cached);
        require(len pieces == 2 && pieces[0] + pieces[1] == changed &&
                pieces[1] == cached.wirebody, "segmented transport shares immutable geometry");
        require(changed[48:] == original[48:] && cached.wirebuilds == 1,
                "Fill/Pen changes reuse committed bytes");
    }
    require(vector->defer(cached, 1, 100, 100) == vector->Ok, "live draft begins");
    preview := vector->wire(cached);
    draftparts := vector->wireparts(cached);
    require(len draftparts == 1 && draftparts[0] == preview, "portable draft wire compatibility");
    require(len preview > len original && cached.wirebuilds == 1, "live draft appends without rebuilding old geometry");
    vector->cancel(cached);
    vector->wire(cached);
    require(cached.wirebuilds == 1, "cancel leaves committed byte cache intact");
    cached.shapes = array[] of {shape, shape};
    changed = vector->wire(cached);
    require(cached.wirebuilds == 2 && len changed > len original, "immutable replacement invalidates at equal revision");
}


# Exercise actual gesture interpolation, immutable history, quotas and strict SVG.
init()
{
    sys = load Sys Sys->PATH;
    vector = load Idevector Idevector->PATH;
    checks = 0;
    require(vector != nil, "load");
    wirecache();
    doc := vector->create();
    require(doc.revision == 0 && doc.saved == 0 && len doc.shapes == 0, "clean document");
    require(vector->pointer(doc, 2, 20, 20) == vector->Conflict, "move without press");
    require(vector->pointer(doc, 1, -1, 20) == vector->Invalid && len doc.points == 0, "invalid coordinate atomic");
    require(vector->pointer(doc, 1, 10, 20) == vector->Ok, "brush press");
    for (i := 0; i < 200; i++) {
        require(vector->pointer(doc, 2, 10 + i * 4, 20 + i * 2) == vector->Ok, "long continuous brush");
    }
    require(len doc.shapes == 0 && doc.revision == 0, "preview is not a committed history unit");
    require(vector->pointer(doc, 3, 900, 500) == vector->Ok, "brush commit");
    require(len doc.shapes == 1 && len doc.shapes[0].nodes <= vector->MaxNodes && doc.nundo == 1,
            "one interpolated bounded path and Undo unit per continuous stroke");
    shape := doc.shapes[0];
    last := shape.nodes[len shape.nodes - 1];
    require(shape.fill && shape.width == 0 && last.verb == vector->Close,
            "brush commits a filled closed contour");
    before := vector->svg(doc);
    require(vector->history(doc, 0) && len doc.shapes == 0, "undo whole stroke");
    require(vector->history(doc, 1) && vector->svg(doc) == before, "redo exact immutable geometry");
    (status, loaded) := vector->parse(before);
    require(status == vector->Ok && vector->svg(loaded) == before, "brush SVG exact roundtrip");
    chain := vector->create();
    chain.tool = vector->Line;
    chain.fill = 1;
    require(vector->pointer(chain, 1, 100, 100) == vector->Ok &&
        vector->pointer(chain, 3, 400, 100) == vector->Ok, "first chain segment");
    require(vector->pointer(chain, 1, 900, 500) == vector->Ok &&
        chain.points[0].x == 400 && chain.points[0].y == 100 &&
        vector->pointer(chain, 3, 400, 400) == vector->Ok, "continue from previous endpoint");
    require(len chain.shapes == 1 && len chain.shapes[0].nodes == 3 &&
        chain.shapes[0].fill == 0 && chain.shapes[0].width == 1 && chain.nundo == 1,
        "unfinished figure is one thin open path and one Undo unit");
    require(vector->pointer(chain, 1, 700, 500) == vector->Ok &&
        vector->pointer(chain, 3, 102, 101) == vector->Ok &&
        len chain.chain == 0 && chain.shapes[0].fill &&
        chain.shapes[0].nodes[len chain.shapes[0].nodes - 1].verb == vector->Close,
        "release at first node snaps, closes and admits fill");
    require(vector->history(chain, 0) && len chain.shapes == 0 &&
        vector->history(chain, 1) && len chain.shapes == 1, "whole figure Undo/Redo");
    require(vector->pointer(chain, 1, 600, 100) == vector->Ok &&
        vector->pointer(chain, 3, 800, 200) == vector->Ok, "new figure after closure");
    vector->finish(chain);
    require(vector->pointer(chain, 1, 650, 300) == vector->Ok &&
        chain.points[0].x == 650 && vector->pointer(chain, 3, 850, 400) == vector->Ok &&
        len chain.shapes == 3, "Finish starts independent next figure");
    native := vector->create();
    vector->preparewire(native);
    builds := native.wirebuilds;
    require(vector->defer(native, 1, 100, 100) == vector->Ok, "native Begin model admission");
    native.nativecapture = 1;
    parts := vector->wireparts(native);
    require(len parts == 2 && parts[1] == "00000000" && native.wirebuilds == builds,
        "native Begin reuses immutable committed body, no portable dot/geometry serialization");
    vector->cancel(native);
    require(!native.nativecapture, "native preview ownership clears on cancel");
    for (tool := vector->Rectangle; tool <= vector->Line; tool++) {
        doc.tool = tool;
        doc.fill = tool != vector->Line;
        require(vector->pointer(doc, 1, 100, 100) == vector->Ok &&
                vector->pointer(doc, 3, 400, 300) == vector->Ok, "figure gesture");
    }
    before = vector->svg(doc);
    (status, loaded) = vector->parse(before);
    if (status != vector->Ok || vector->svg(loaded) != before || len loaded.shapes != 4) {
        sys->print("SVG DEBUG status=%d expected=%s\n", status, before);
        if (loaded != nil) {
            sys->print("SVG ACTUAL %s\n", vector->svg(loaded));
        }
    }
    require(status == vector->Ok && vector->svg(loaded) == before && len loaded.shapes == 4,
            "all tool geometry and styles roundtrip");
    require(vector->pointer(doc, 1, 5, 5) == vector->Ok, "cancel press");
    require(!vector->history(doc, 0), "history refuses active preview");
    vector->cancel(doc);
    require(vector->svg(doc) == before && len doc.points == 0, "cancel leaves committed source unchanged");
    samples := array[] of {
        "<rect x='10' y='20' width='30' height='40' fill='#ff0000'/>",
        "<ellipse cx='400' cy='300' rx='100' ry='80' fill='none' stroke='#229955' stroke-width='8'/>",
        "<line x1='10' y1='20' x2='900' y2='500' fill='none' stroke='#222222'/>"};
    for (i = 0; i < len samples; i++) {
        (status, loaded) = vector->parse("<svg>" + samples[i] + "</svg>");
        require(status == vector->Ok && len loaded.shapes == 1, "ordinary SVG primitive import");
        canonical := vector->svg(loaded);
        (status, again) := vector->parse(canonical);
        require(status == vector->Ok && vector->svg(again) == canonical, "import canonical roundtrip including fill-only");
    }
    bad := array[] of {
        "<svg><script/></svg>", "<!DOCTYPE svg><svg></svg>",
        "<svg><path d='M 0 0 L 1 1' onclick='run()'/></svg>",
        "<svg><path d='M 0 0 C 1 1 2 2 3 3'/></svg>",
        "<svg><path d='M 0 0 L 1001 0'/></svg>",
        "<svg><rect width='20.5' height='30'/></svg>",
        "<svg><rect width='20' width='30' height='30'/></svg>",
        "<svg><rect width='20' height='30' fill='#ff0000' stroke='#000000'/></svg>",
        "<svg><rect width='20' height='30' transform='rotate(30)'/></svg>",
        "<svg></svg>trailing", "<svg viewBox='0 0 20 20'></svg>"};
    for (i = 0; i < len bad; i++) {
        (status, loaded) = vector->parse(bad[i]);
        require(status != vector->Ok && loaded == nil, "unsupported SVG refused atomically");
    }
    for (i = 2; i <= 16; i++) {
        (status, loaded) = vector->parse(before[0:len before - i]);
        require(status != vector->Ok && loaded == nil, "truncated SVG never publishes partial document");
    }
    dot := vector->create();
    require(vector->pointer(dot, 1, 100, 100) == vector->Ok &&
            vector->pointer(dot, 3, 100, 100) == vector->Ok, "single-click brush");
    require(dot.shapes[0].fill && dot.shapes[0].width == 0 && len dot.shapes[0].nodes >= 4,
            "single click becomes a visible filled dot");
    (status, loaded) = vector->parse(vector->svg(dot));
    require(status == vector->Ok, "dot SVG roundtrip");
    dot.width = 0;
    require(vector->pointer(dot, 1, 20, 20) == vector->Invalid && len dot.points == 0,
            "invalid authoring style refused atomically");
    # Limbo strings hold Unicode scalars; the BOM is one U+FEFF, not three bytes.
    metadata := "";
    metadata[len metadata] = 16rfeff;
    metadata += "<?xml version='1.0' encoding='UTF-8'?><!-- drawing π -->"
        + "<svg width='1000.0' height='600.000' viewBox='0  0 1000.0 600'>"
        + "<!-- leaf --><rect x='10.0' y='20.00' width='30.000' height='40' fill='#ff0000'/></svg><!-- done -->";
    (status, loaded) = vector->parse(metadata);
    require(status == vector->Ok && len loaded.shapes == 1 && loaded.shapes[0].nodes[0].x == 10,
            "BOM, UTF8 declaration, comments and exact zero fractions preserve geometry");
    (status, loaded) = vector->parse("<svg/>");
    require(status == vector->Ok && len loaded.shapes == 0, "self-closing empty canvas");
    invalidmeta := array[] of {"<!-- bad -- comment --><svg/>", "<!-- unterminated<svg/>",
        "<?xml version='1.0' encoding='ISO-8859-1'?><svg/>", "<?xml version='1.1'?><svg/>",
        "<!-- prefix --><?xml version='1.0'?><svg/>", "<svg><!-- open</svg>",
        "<svg><path d='M 0 0 L 1.1 2'/></svg>",
        "<svg><path d='M 0 0 L 1<!-- not path trivia --> 2'/></svg>"};
    for (i = 0; i < len invalidmeta; i++) {
        (status, loaded) = vector->parse(invalidmeta[i]);
        require(status != vector->Ok && loaded == nil, "invalid metadata or lossy geometry refused atomically");
    }
    utf8 := array[vector->MaxSVG + 2] of byte;
    for (i = 0; i < len utf8; i += 2) {
        utf8[i] = byte 16rce;
        utf8[i+1] = byte 16ra9;
    }
    (status, loaded) = vector->parse("<!--" + string utf8 + "--><svg/>");
    require(status == vector->Limit && loaded == nil, "SVG byte quota includes multibyte comment metadata");
    crowded := vector->create();
    crowded.tool = vector->Rectangle;
    for (i = 0; i < vector->MaxShapes; i++) {
        require(vector->pointer(crowded, 1, 10, 10) == vector->Ok &&
                vector->pointer(crowded, 3, 20, 20) == vector->Ok, "bounded figure capacity");
    }
    before = vector->svg(crowded);
    require(vector->pointer(crowded, 1, 20, 20) != vector->Ok && vector->svg(crowded) == before,
            "path capacity refusal preserves document");
    require(len vector->wire(crowded) <= 22000 && crowded.nundo == 16, "wire/history bounded");
    sys->print("PASS vector model: %d predicates, actual gestures, curves, history, SVG and refusal atomicity\n", checks);
}
