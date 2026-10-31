implement IdepaintTest;
include "sys.m";
include "idevector.m";
sys: Sys; # Test diagnostics and explicit failure control.
v: Idevector; # Actual model; no duplicated geometry implementation.


# Exercise post-release processing independently of native graphics.
IdepaintTest: module {
    init: fn();
};


# Keep predicates enabled in release builds; message identifies the failed contract.
require(ok: int, message: string)
{
    if (!ok) {
        sys->print("FAIL paint: %s\n", message);
        fd := sys->open("/dev/sysctl", Sys->OWRITE);
        data := array of byte "halt";
        if (fd != nil) {
            sys->write(fd, data, len data);
        }
        raise "fail: paint";
    }
}


# Commit one raw gesture without invoking the expensive worker path.
# doc belongs to this test process; all coordinates are logical canvas units.
line(doc: ref Idevector->Document, x, y, endx, endy: int)
{
    require(v->defer(doc, 1, x, y) == v->Ok, "press");
    require(v->defer(doc, 3, endx, endy) == v->Ok, "raw release");
}


# Inspect the canonical filled-cell result without depending on contour simplification.
# area is immutable borrowed coverage; x/y are strictly inside the page.
ink(area: array of byte, x, y: int): int
{
    cell := y * 1000 + x;
    return (int area[cell >> 3] & (1 << (cell & 7))) != 0;
}


# Verify distinct tools, delayed smoothing, compound holes, same-colour union,
# immutable history and processing of multiple queued raw gestures in one snapshot.
init()
{
    sys = load Sys Sys->PATH;
    v = load Idevector Idevector->PATH;
    require(v != nil, "load model");
    pen := v->create();
    pen.tool = v->Pen;
    require(v->defer(pen, 1, 100, 100) == v->Ok, "pen press");
    for (i := 1; i <= 120; i++) {
        require(v->defer(pen, 2, 100 + i, 100 + i % 2) == v->Ok, "pen sample");
    }
    require(pen.used == 121 && pen.points[1].x == 101, "no sample decimation");
    require(v->defer(pen, 3, 240, 110) == v->Ok, "pen raw commit");
    raw := pen.shapes[0];
    require(raw.nodes[1].verb == v->Segment && raw.author == v->Pen, "no live smoothing");
    require(v->refine(pen) == v->Ok, "pen final refinement");
    shape := pen.shapes[0];
    last := shape.nodes[len shape.nodes - 1];
    require(shape.nodes[1].verb == v->Quadratic && !shape.fill && shape.width == 8 &&
            last.x == 240 && last.y == 110 && raw.nodes[1].verb == v->Segment,
            "pen curves preserve endpoint, width and immutable raw input");
    brush := v->create();
    brush.width = 12;
    require(v->defer(brush, 1, 100, 100) == v->Ok, "closed brush press");
    require(v->defer(brush, 2, 160, 100) == v->Ok, "top");
    require(v->defer(brush, 2, 160, 160) == v->Ok, "right");
    require(v->defer(brush, 2, 100, 160) == v->Ok, "bottom");
    require(v->defer(brush, 3, 100, 100) == v->Ok, "closed release");
    require(brush.shapes[0].area == nil, "raw trace commits before vectorization");
    require(v->refine(brush) == v->Ok, "brush contour");
    shape = brush.shapes[0];
    require(shape.fill && shape.width == 0 && ink(shape.area, 120, 100) &&
            !ink(shape.area, 130, 130), "filled nib leaves enclosed hole empty");
    moves := 0;
    for (i = 0; i < len shape.nodes; i++) {
        moves += shape.nodes[i].verb == v->Move;
    }
    require(moves == 2, "compound outer and hole contours");
    (status, opened) := v->parse(v->svg(brush));
    require(status == v->Ok && v->svg(opened) == v->svg(brush), "compound SVG roundtrip");
    before := v->svg(brush);
    original := shape.area;
    line(brush, 160, 130, 190, 130);
    line(brush, 190, 130, 210, 130);
    require(len brush.shapes == 3, "multiple raw jobs remain visible");
    require(v->refine(brush) == v->Ok && len brush.shapes == 1,
            "latest snapshot processes and merges both queued gestures");
    require(ink(brush.shapes[0].area, 205, 130) && !ink(original, 205, 130),
            "union does not mutate old coverage");
    require(v->history(brush, 0), "undo last stroke");
    status = v->refine(brush);
    require(status == v->Ok && !ink(brush.shapes[0].area, 205, 130),
            "undo restores prior gesture rather than refined union alias");
    require(v->history(brush, 0) && v->svg(brush) == before, "undo whole connected addition");
    brush.color = 16rff0000;
    line(brush, 100, 100, 120, 100);
    require(v->refine(brush) == v->Ok && len brush.shapes == 2, "different colours stay separate");
    dot := v->create();
    dot.tool = v->Pen;
    line(dot, 20, 20, 20, 20);
    require(v->refine(dot) == v->Ok && dot.shapes[0].author == 0 && dot.shapes[0].fill,
            "pen dot finishes without remaining a pending raw job");
    bounded := v->create();
    bounded.tool = v->Pen;
    require(v->defer(bounded, 1, 0, 0) == v->Ok, "capacity press");
    for (i = 1; i < v->MaxNodes; i++) {
        require(v->defer(bounded, 2, i % 1000, i % 600) == v->Ok, "capacity sample");
    }
    require(v->defer(bounded, 2, 999, 599) == v->Limit && bounded.used == 0 &&
            len bounded.shapes == 1 && len bounded.shapes[0].nodes == v->MaxNodes &&
            bounded.nundo == 1, "capacity preserves the visible trace as one raw history unit");
    sys->print("PASS paint: raw pen, final curves, brush fill/hole, union, queued gestures and immutable Undo\n");
}
