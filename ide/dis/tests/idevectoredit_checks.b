# Check owner-local editing transactions against exact immutable source commands.
editing()
{
    d := vector->create();
    d.tool = vector->Rectangle;
    d.fill = 1;
    vector->pointer(d, 1, 100, 100);
    vector->pointer(d, 3, 200, 200);
    d.tool = vector->Select;
    original := d.shapes;
    vector->pointer(d, 1, 150, 150);
    vector->pointer(d, 2, 170, 180);
    vector->pointer(d, 3, 180, 190);
    require(len d.shapes == 1 && d.shapes[0].nodes[0].x == 130 &&
            d.shapes[0].nodes[0].y == 140, "object translation from down snapshot");
    require(original[0].nodes[0].x == 100 && d.revision == 2 && d.nundo == 2,
            "immutable baseline and one history unit");
    vector->pointer(d, 1, 230, 240);
    vector->pointer(d, 3, 330, 340);
    require(d.shapes[0].nodes[2].x == 330, "opposite-corner scale");
    vector->history(d, 0);
    require(d.shapes[0].nodes[2].x == 230, "Undo restores exact commands");
    vector->pointer(d, 1, 180, 122);
    vector->pointer(d, 3, 248, 190);
    require(d.shapes[0].nodes[0].x >= 229 && d.shapes[0].nodes[0].x <= 231 &&
            d.shapes[0].nodes[0].y >= 139 && d.shapes[0].nodes[0].y <= 141,
            "rotation handle turns around figure centre");
    vector->history(d, 0);
    d.tool = vector->Nodes;
    vector->pointer(d, 1, 130, 140);
    vector->pointer(d, 3, 140, 150);
    require(d.shapes[0].nodes[0].x == 140 && d.shapes[0].nodes[1].x == 230,
            "one node independent of another");
    d.control = 1;
    vector->pointer(d, 1, 230, 140);
    vector->pointer(d, 3, 230, 140);
    d.control = 0;
    vector->pointer(d, 1, 140, 150);
    vector->pointer(d, 3, 150, 160);
    require(d.shapes[0].nodes[0].x == 150 && d.shapes[0].nodes[1].x == 240,
            "Ctrl selects node group");
    vector->pointer(d,1,150,160);
    held := d.shapes;
    vector->pointer(d,2,0,0);
    require(d.shapes == held && d.edit.applied.x == 150 && d.edit.applied.y == 160 &&
        d.edit.current.x == 0,"local baseline pointer follows valid geometry rather than refused group motion");
    vector->cancel(d);
    vector->pointer(d, 1, 130, 130);
    vector->pointer(d, 2, 260, 180);
    vector->pointer(d, 3, 260, 180);
    require(d.edit.mask[0] && d.edit.mask[3] && !d.edit.mask[6],
            "marquee selects only enclosed nodes");
    unchanged := d.shapes;
    count := d.nundo;
    vector->pointer(d, 1, 150, 160);
    vector->pointer(d, 2, 160, 170);
    vector->pointer(d, 3, 150, 160);
    require(d.shapes == unchanged && d.nundo == count, "return to origin is not an edit");
    firstparts := vector->wireparts(d);
    builds := d.presentation.builds;
    body := d.presentation.paths;
    for (snapshot := 0; snapshot < 20; snapshot++) {
        vector->wireparts(d);
    }
    vector->cancel(d);
    vector->wireparts(d);
    require(d.presentation.builds == builds && d.presentation.paths == body && len firstparts == 4,
            "unchanged snapshots/cancel reuse source and overlay presentation");
    before := d.shapes;
    vector->pointer(d, 1, 150, 160);
    vector->pointer(d, 2, 160, 170);
    vector->cancel(d);
    require(d.shapes == before, "Cancel restores immutable baseline");
    ellipse := vector->create();
    ellipse.tool = vector->Ellipse;
    vector->pointer(ellipse, 1, 300, 200);
    vector->pointer(ellipse, 3, 500, 400);
    ellipse.tool = vector->Nodes;
    vector->pointer(ellipse, 1, 500, 300);
    vector->pointer(ellipse, 3, 500, 300);
    curve := ellipse.shapes[0].nodes[1];
    vector->pointer(ellipse, 1, curve.cx, curve.cy);
    vector->pointer(ellipse, 3, curve.cx-10, curve.cy+10);
    require(ellipse.shapes[0].nodes[1].verb == vector->Cubic &&
            ellipse.shapes[0].nodes[1].cx == curve.cx-10 &&
            ellipse.shapes[0].nodes[1].x == curve.x, "first cubic control preserves endpoint");
    first := ellipse.shapes[0].nodes[0];
    vector->pointer(ellipse,1,first.x,first.y);
    vector->pointer(ellipse,3,first.x+10,first.y);
    closed := ellipse.shapes[0].nodes;
    require(closed[0].x == closed[4].x && closed[0].x == first.x+10,
            "closed cubic anchor copies stay welded during node editing");
}


# Check relative brush fitting and camera-only transactions using exact source
# values. Both quadratic controls remain ordinary editable contour fields.
brushviewport()
{
    points := array[65] of Idevector->Point;
    math := load Math Math->PATH;
    require(math != nil,"fitting math module");
    for (i := 0; i < len points; i++) {
        angle := real i*6.283185307179586/64.0;
        points[i] = Idevector->Point(500+int (300.0*math->cos(angle)),
            300+int (180.0*math->sin(angle)));
    }
    points[64] = points[0];
    fitted := vector->brushfit(points,600,360);
    require(fitted != nil && len fitted <= 10 && fitted[1].verb == vector->Cubic,
            "global oval fit uses a few cubics instead of dense local knots");
    d := vector->create();
    d.tool = vector->Ellipse;
    vector->pointer(d,1,200,120);
    vector->pointer(d,3,800,480);
    require(len d.shapes[0].nodes == 6,"ellipse has four cubic segments and Close");
    d.tool = vector->Nodes;
    vector->pointer(d,1,800,300);
    vector->pointer(d,3,800,300);
    handle := d.shapes[0].nodes[1];
    vector->pointer(d,1,handle.c2x,handle.c2y);
    vector->pointer(d,3,handle.c2x+2,handle.c2y+3);
    require(d.shapes[0].nodes[1].c2x == handle.c2x+2 &&
            d.shapes[0].nodes[1].cx == handle.cx,
            "second cubic handle moves independently of first");
    (parsed,roundtrip) := vector->parse(vector->svg(d));
    require(parsed == vector->Ok && vector->svg(roundtrip) == vector->svg(d),
            "absolute cubic SVG round trip preserves both controls");
    d.tool = vector->Zoom;
    source := vector->svg(d);
    vector->pointer(d,1,200,100);
    vector->pointer(d,2,450,250);
    vector->pointer(d,3,450,250);
    require(d.zoom == 400 && d.center.x == 325 && d.center.y == 175 && d.revision == 2 && d.nundo == 2,
            "marquee fits and centres camera without a source transaction");
    wire := vector->wire(d);
    require(wire[0:8] == "00030008" && vector->svg(d) == source,
            "nondefault camera is private presentation metadata only");
    vector->viewport(d,1,325,175);
    require(d.zoom == 500 && d.center.x == 325 && d.center.y == 175, "focused wheel camera step");
    vector->viewport(d,0,0,0);
    require(d.zoom == 100 && d.center.x == 500 && d.center.y == 300 && vector->svg(d) == source,
            "1:1 reset preserves source");
}


# Check independent selection policy against nested fills and nearby open paths.
# No native hit routine or flattened oracle is substituted for the model.
picking()
{
    d := vector->create();
    d.tool = vector->Rectangle;
    d.fill = 1;
    vector->pointer(d,1,100,100);
    vector->pointer(d,3,600,500);
    vector->pointer(d,1,200,200);
    vector->pointer(d,3,400,400);
    d.tool = vector->Select;
    vector->pointer(d,1,300,300);
    vector->pointer(d,3,300,300);
    require(d.edit.selected == 1,"topmost containing nested figure wins");
    d.tool = vector->Nodes;
    vector->pointer(d,1,300,300);
    vector->pointer(d,3,300,300);
    require(d.edit.selected == 1,"Nodes retains selected contour on an interior click");
    lines := vector->create();
    lines.tool = vector->Line;
    vector->pointer(lines,1,100,100);
    vector->pointer(lines,3,800,100);
    vector->finish(lines);
    vector->pointer(lines,1,100,130);
    vector->pointer(lines,3,800,130);
    vector->finish(lines);
    lines.tool = vector->Select;
    vector->pointer(lines,1,400,111);
    vector->pointer(lines,3,400,111);
    require(lines.edit.selected == 0,"globally nearer contour beats topmost farther path");
    vector->pointer(lines,1,400,151);
    vector->pointer(lines,3,400,151);
    require(lines.edit.selected == -1,"contours beyond20 logical units are refused");
    lines.pickradius = 5.0;
    vector->pointer(lines,1,400,106);
    vector->pointer(lines,3,400,106);
    require(lines.edit.selected == -1,"copied screen conversion controls zoomed tolerance");
    lines.pickradius = 20.0;
    vector->pointer(lines,1,400,150);
    vector->pointer(lines,3,400,150);
    require(lines.edit.selected == 1,"20-unit contour boundary is inclusive");
}
