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


# EN: Select exactly at page point, then discard the pending gesture.
# RU: Выбрать фигуру точно в точке листа и снять незавершённый жест.
atpoint(doc: ref Idevector->Document, x, y: int): int
{
    require(v->pointer(doc, 1, x, y) == v->Ok, "point pick");
    chosen := doc.edit.selected;
    v->cancel(doc);
    return chosen;
}


# EN: A stroke cuts an opaque lower fill across its whole width. Move the
# top stroke away, check the exposed gap and the two independently picked
# islands, then undo both the move and the entire paint transaction.
# RU: Мазок разрезает непрозрачную нижнюю заливку. После переноса мазка
# проверить просвет и независимое выделение островков, затем отменить
# перемещение и весь мазок.
mergecut()
{
    start := "<svg width=\"1000\" height=\"600\"><rect x=\"100\" y=\"100\" width=\"200\" height=\"120\" fill=\"#e04444\"/></svg>";
    (status, doc) := v->parse(start);
    require(status == v->Ok, "read Merge Drawing target");
    original := v->svg(doc);
    doc.width = 20;
    line(doc, 60, 160, 340, 160);
    require(v->refine(doc) == v->Ok && len doc.shapes == 3, "paint splits lower fill into two islands");
    top := doc.shapes[2];
    require(top.fill && len top.nodes == 8 && top.nodes[2].verb == v->Cubic &&
            top.nodes[6].verb == v->Cubic, "straight brush keeps four cubic cap arcs");
    sliced := v->svg(doc);
    (status, doc) = v->parse(sliced);
    require(status == v->Ok && len doc.shapes == 3 && v->svg(doc) == sliced, "cut SVG roundtrip");
    doc.tool = v->Select;
    doc.pickradius = 0.0;
    require(v->pointer(doc, 1, 196, 160) == v->Ok && doc.edit.selected == 2 &&
            v->pointer(doc, 3, 196, 260) == v->Ok, "move stroke away");
    require(atpoint(doc, 180, 160) == -1, "moved stroke leaves a real gap");
    upper := atpoint(doc, 180, 130);
    lower := atpoint(doc, 180, 190);
    require(upper >= 0 && lower >= 0 && upper != lower &&
            doc.shapes[upper].color == 16re04444 && doc.shapes[lower].color == 16re04444,
            "cut islands select independently");
    require(v->history(doc, 0) && v->svg(doc) == sliced, "undo stroke move");
    (status, doc) = v->parse(start);
    require(status == v->Ok, "read undo baseline");
    line(doc, 60, 160, 340, 160);
    require(v->refine(doc) == v->Ok && v->history(doc, 0) && v->svg(doc) == original,
            "undo cut restores source geometry");
}


# EN: Verify that an interior dot creates a cubic hole while retaining
# the exact handles and control-join metadata of the old contour.
# RU: Проверить кубическое отверстие внутри фигуры и сохранение точных
# ручек и типов стыков исходного контура.
mergehole()
{
    start := "<svg width=\"1000\" height=\"600\"><path fill=\"#e04444\" data-flashvm-joins=\"010000\" d=\"M 100 100 C 140 50 260 50 300 100 L 300 240 L 100 240 L 100 100 Z\"/></svg>";
    (status, doc) := v->parse(start);
    require(status == v->Ok, "read original cubic fill");
    old := doc.shapes[0];
    doc.width = 20;
    line(doc, 200, 160, 200, 160);
    require(v->refine(doc) == v->Ok && len doc.shapes == 2, "interior brush dot makes cut");
    cut := doc.shapes[0];
    require(len cut.nodes == len old.nodes + 6 && cut.nodes[1].cx == 140 &&
            cut.nodes[1].c2x == 260 && cut.nodes[1].cy == 50 && cut.joins[1] == 1,
            "interior cut preserves old cubic and join");
    require(!ink(cut.area, 200, 160) && ink(cut.area, 200, 135), "true coloured hole");
    require(len doc.shapes[1].nodes == 6 && doc.shapes[1].nodes[4].verb == v->Cubic,
            "dot remains four cubics");
    require(v->history(doc, 0) && len doc.shapes == 1 && doc.shapes[0] == old,
            "hole Undo restores exact original object");
    doc.color = old.color;
    line(doc, 190, 160, 210, 160);
    require(v->refine(doc) == v->Ok && len doc.shapes == 1 && doc.shapes[0] == old,
            "painting inside identical colour keeps source contour");
}


# EN: Include an outlined fill and discard a fully covered neighbour.
# RU: Включить фигуру с обводкой и удалить полностью закрашенного соседа.
mergeoutlined()
{
    start := "<svg width=\"1000\" height=\"600\"><rect x=\"100\" y=\"100\" width=\"40\" height=\"40\" fill=\"#245ac4\" stroke=\"#245ac4\" stroke-width=\"4\"/><rect x=\"161\" y=\"117\" width=\"6\" height=\"6\" fill=\"#e04444\"/></svg>";
    (status, doc) := v->parse(start);
    require(status == v->Ok && doc.shapes[0].width == 4, "read fill with same-colour outline");
    doc.width = 20;
    line(doc, 120, 120, 175, 120);
    require(v->refine(doc) == v->Ok && len doc.shapes == 1 && doc.shapes[0].color == doc.color &&
            ink(doc.shapes[0].area, 110, 125) && ink(doc.shapes[0].area, 165, 120),
            "merge outlined fill and remove fully overpainted rectangle");
    require(v->history(doc, 0) && len doc.shapes == 2, "undo restores completely covered figure");
}


# EN: Check the edited side curves and the untouched upper and lower cubics.
# RU: Проверить разрезанные боковые кривые и неизменные верхнюю и нижнюю кубики.
curveparts(doc: ref Idevector->Document, color: int): (int, int, int, int)
{
    quadratics := 0;
    side := 0;
    top := 0;
    bottom := 0;
    for (i := 0; i < len doc.shapes; i++) {
        shape := doc.shapes[i];
        if (shape.color != color) {
            continue;
        }
        for (j := 0; j < len shape.nodes; j++) {
            node := shape.nodes[j];
            quadratics += node.verb == v->Quadratic;
            if (node.verb == v->Cubic) {
                side += node.cx >= 400 && node.cx <= 445 && node.c2x >= 400 && node.c2x <= 445;
                top += node.x == 400 && node.y == 100 && node.cx == 170 && node.cy == 40 &&
                    node.c2x == 330 && node.c2y == 40;
                bottom += node.x == 100 && node.y == 240 && node.cx == 325 && node.cy == 300 &&
                    node.c2x == 175 && node.c2y == 300;
            }
        }
    }
    return (quadratics, side, top, bottom);
}


# EN: Cross a quadratic and a cubic twice, retaining two other cubics verbatim in
# independent islands. A same-colour sweep then exercises direct union.
# RU: Пересечь квадратичный и кубический сегменты дважды, сохраняя другие кубики
# в отдельных островках. Мазок того же цвета проверяет прямое объединение.
mergecurves()
{
    start := "<svg width=\"1000\" height=\"600\"><path fill=\"#e04444\" d=\"M 100 100 C 170 40 330 40 400 100 C 445 135 445 205 400 240 C 325 300 175 300 100 240 Q 48 170 100 100 Z\"/></svg>";
    (status, doc) := v->parse(start);
    require(status == v->Ok, "read curved Merge Drawing target");
    source := v->svg(doc);
    doc.width = 20;
    line(doc, 50, 170, 480, 170);
    require(v->refine(doc) == v->Ok && len doc.shapes == 3, "vector difference splits curved fill");
    (quads, side, top, bottom) := curveparts(doc, 16re04444);
    require(quads == 2 && side == 2 && top == 1 && bottom == 1, "difference retains exact handles and splits Q/C sides");
    saved := v->svg(doc);
    reopened: ref Idevector->Document;
    (status, reopened) = v->parse(saved);
    require(status == v->Ok && v->svg(reopened) == saved, "vector cut survives SVG roundtrip");
    require(v->history(doc, 0) && v->svg(doc) == source, "undo direct vector cut");
    doc.color = 16re04444;
    line(doc, 280, 170, 480, 170);
    require(v->refine(doc) == v->Ok && len doc.shapes == 1, "same-colour vector union");
    (quads, side, top, bottom) = curveparts(doc, doc.color);
    require(quads == 1 && side == 2 && top == 1 && bottom == 1 && ink(doc.shapes[0].area, 470, 170),
            "union keeps untouched cubics and joins the extended round nib");
}


# EN: Two turns and a long horizontal terminal leg preserve the exact cap.
# RU: Два поворота и горизонтальный конец сохраняют точное закругление.
mergebend()
{
    doc := v->create();
    doc.width = 20;
    require(v->defer(doc, 1, 80, 190) == v->Ok && v->defer(doc, 2, 160, 130) == v->Ok &&
            v->defer(doc, 2, 250, 125) == v->Ok && v->defer(doc, 2, 340, 175) == v->Ok &&
            v->defer(doc, 3, 420, 175) == v->Ok, "release bent brush sweep");
    require(v->refine(doc) == v->Ok && len doc.shapes == 1, "refine bent stroke");
    nodes := doc.shapes[0].nodes;
    cap := 0;
    arcs := 0;
    for (i := 0; i < len nodes; i++) {
        node := nodes[i];
        if (node.verb == v->Cubic) {
            arcs++;
            cap += node.x == 430 && node.y == 175 && node.cx == 426 && node.cy == 165 &&
                node.c2x == 430 && node.c2y == 169;
        }
    }
    require(cap == 1 && arcs == 7 && ink(doc.shapes[0].area, 250, 125),
            "bent brush keeps cubic caps and three outer circular joins");
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
    require(v->refine(brush) == v->Ok && len brush.shapes > 1, "overpaint leaves some blue fill");
    lastindex := len brush.shapes - 1;
    require(brush.shapes[lastindex].color == brush.color && ink(brush.shapes[lastindex].area, 110, 100),
            "red owns the overpainted point");
    for (i = 0; i < lastindex; i++) {
        require(brush.shapes[i].color == shape.color, "remaining islands keep their blue colour");
    }
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
    mergecut();
    mergehole();
    mergeoutlined();
    mergecurves();
    mergebend();
    sys->print("PASS vector Merge Drawing: retained C handles, split Q/C sides and swept cubic joins\n");
    sys->print("PASS Merge Drawing: destructive cut, independent islands, cubic hole, move and Undo\n");
    sys->print("PASS paint: raw pen, final curves, brush fill/hole, union, queued gestures and immutable Undo\n");
}
