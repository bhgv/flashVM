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
    vector->pointer(d, 1, 130, 130);
    vector->pointer(d, 2, 260, 180);
    vector->pointer(d, 3, 260, 180);
    require(d.edit.mask[0] && d.edit.mask[2] && !d.edit.mask[4],
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
    require(ellipse.shapes[0].nodes[1].verb == vector->Quadratic &&
            ellipse.shapes[0].nodes[1].cx == curve.cx-10 &&
            ellipse.shapes[0].nodes[1].x == curve.x, "quadratic control preserves endpoint");
}


# Check relative brush fitting and camera-only transactions using exact source
# values. Both quadratic controls remain ordinary editable contour fields.
brushviewport()
{
    points := array[] of {Idevector->Point(100,100),Idevector->Point(103,102),
        Idevector->Point(106,104),Idevector->Point(109,105),Idevector->Point(112,106),
        Idevector->Point(800,100),Idevector->Point(800,500),Idevector->Point(100,100)};
    fitted := vector->brushfit(points,700,400);
    require(fitted != nil && len fitted < len points && fitted[1].verb == vector->Quadratic &&
            fitted[2].verb == vector->Quadratic, "dense 1/40 cluster uses a two-handle spline");
    require(fitted[1].x == 106 && fitted[1].y == 104 && fitted[2].x == 112,
            "spline interpolates endpoints and middle knot");
    d := vector->create();
    d.shapes = array[] of {ref Idevector->Shape(16r245ac4,1,0,fitted,nil,0)};
    d.tool = vector->Nodes;
    vector->pointer(d,1,100,100);
    vector->pointer(d,3,100,100);
    handle := fitted[1];
    vector->pointer(d,1,handle.cx,handle.cy);
    vector->pointer(d,3,handle.cx+2,handle.cy+3);
    require(d.shapes[0].nodes[1].cx == handle.cx+2 && d.shapes[0].nodes[2].cx == fitted[2].cx,
            "nearest fitted spline handle moves independently");
    d.tool = vector->Zoom;
    source := vector->svg(d);
    vector->pointer(d,1,200,100);
    vector->pointer(d,2,450,250);
    vector->pointer(d,3,450,250);
    require(d.zoom == 400 && d.center.x == 325 && d.center.y == 175 && d.revision == 1 && d.nundo == 1,
            "marquee fits and centres camera without a source transaction");
    wire := vector->wire(d);
    require(wire[0:8] == "00010008" && vector->svg(d) == source,
            "nondefault camera is private presentation metadata only");
    vector->viewport(d,1,325,175);
    require(d.zoom == 500 && d.center.x == 325 && d.center.y == 175, "focused wheel camera step");
    vector->viewport(d,0,0,0);
    require(d.zoom == 100 && d.center.x == 500 && d.center.y == 300 && vector->svg(d) == source,
            "1:1 reset preserves source");
}
