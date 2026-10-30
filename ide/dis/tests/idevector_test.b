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


# Exercise actual gesture interpolation, immutable history, quotas and strict SVG.
init()
{
    sys = load Sys Sys->PATH;
    vector = load Idevector Idevector->PATH;
    checks = 0;
    require(vector != nil, "load");
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
    require(shape.nodes[1].verb == vector->Quadratic && last.x == 900 && last.y == 500,
            "quadratic interpolation and final endpoint");
    before := vector->svg(doc);
    require(vector->history(doc, 0) && len doc.shapes == 0, "undo whole stroke");
    require(vector->history(doc, 1) && vector->svg(doc) == before, "redo exact immutable geometry");
    (status, loaded) := vector->parse(before);
    require(status == vector->Ok && vector->svg(loaded) == before, "brush SVG exact roundtrip");
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
    require(dot.shapes[0].fill && dot.shapes[0].width == 0 && len dot.shapes[0].nodes == 10,
            "single click becomes a visible filled dot");
    (status, loaded) = vector->parse(vector->svg(dot));
    require(status == vector->Ok, "dot SVG roundtrip");
    dot.width = 0;
    require(vector->pointer(dot, 1, 20, 20) == vector->Invalid && len dot.points == 0,
            "invalid authoring style refused atomically");
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
