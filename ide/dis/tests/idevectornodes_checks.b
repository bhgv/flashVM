# Exercise actual public contour commands, immutable history and tangent movement.


# Build a real owner-selected open three-anchor cubic path through SVG parsing.
# Every new fixture has its own history and no native presentation dependency.
nodefixture(): ref Idevector->Document
{
    (status,d) := vector->parse("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1000 600\"><path fill=\"none\" stroke=\"#245ac4\" stroke-width=\"1\" d=\"M 100 200 C 150 200 220 150 300 200 C 340 250 450 200 500 200\"/></svg>");
    require(status == vector->Ok, "node fixture SVG parse");
    d.tool = vector->Nodes;
    vector->pointer(d,1,300,200);
    vector->pointer(d,3,300,200);
    vector->pointer(d,1,300,200);
    vector->pointer(d,3,300,200);
    return d;
}


# All nine operations must alter the actual document or explicitly refuse an
# invalid selection, with one transaction per command and exact Undo restoration.
nodeoperations()
{
    d := nodefixture();
    before := d.shapes;
    revision := d.revision;
    require(vector->nodeoperation(d,vector->AddNode) == vector->Ok && len d.shapes[0].nodes == 4,
            "Add splits the following cubic");
    require(d.shapes[0].nodes[2].x == 396 && d.revision == revision+1,
            "de Casteljau midpoint and one revision");
    vector->history(d,0);
    require(d.shapes == before, "node operation Undo restores exact immutable array");
    d = nodefixture();
    require(vector->nodeoperation(d,vector->DeleteNode) == vector->Ok && len d.shapes[0].nodes == 2,
            "Delete removes selected anchor");
    d = nodefixture();
    require(vector->nodeoperation(d,vector->BreakNode) == vector->Ok &&
            d.shapes[0].nodes[2].verb == vector->Move && d.shapes[0].nodes[2].x == 300,
            "Break duplicates the anchor for detached subpath");
    require(vector->nodeoperation(d,vector->JoinNodes) == vector->Ok && len d.shapes[0].nodes == 3,
            "Join welds the two split endpoints");
    d = nodefixture();
    d.edit.mask[6] = 1;
    require(vector->nodeoperation(d,vector->StraightSpan) == vector->Ok &&
            d.shapes[0].nodes[2].verb == vector->Segment, "selected span becomes a line");
    require(vector->nodeoperation(d,vector->BezierSpan) == vector->Ok &&
            d.shapes[0].nodes[2].verb == vector->Cubic, "selected line becomes cubic");
    d = nodefixture();
    require(vector->nodeoperation(d,vector->SmoothNode) == vector->Ok &&
            d.shapes[0].joins[1] == 1, "smooth mode stored with immutable shape");
    vector->pointer(d,1,d.shapes[0].nodes[2].cx,d.shapes[0].nodes[2].cy);
    vector->pointer(d,3,300,300);
    require(d.shapes[0].nodes[1].c2x == 300 &&
            d.shapes[0].nodes[1].c2y >= 104 && d.shapes[0].nodes[1].c2y <= 107,
            "smooth handle rotates opposite arm preserving independent length");
    d = nodefixture();
    require(vector->nodeoperation(d,vector->SymmetricNode) == vector->Ok &&
            d.shapes[0].joins[1] == 2, "symmetric mode stored");
    vector->pointer(d,1,d.shapes[0].nodes[2].cx,d.shapes[0].nodes[2].cy);
    vector->pointer(d,3,300,300);
    require(d.shapes[0].nodes[1].c2x == 300 && d.shapes[0].nodes[1].c2y == 100,
            "symmetric handles rotate and resize together");
    saved := vector->svg(d);
    (loadedstatus,loaded) := vector->parse(saved);
    require(loadedstatus == vector->Ok && loaded.shapes[0].joins[1] == 2,
            "SVG retains explicit symmetric node metadata");
    d = vector->create();
    d.tool = vector->Ellipse;
    vector->pointer(d,1,100,100);
    vector->pointer(d,3,500,400);
    d.tool = vector->Nodes;
    vector->pointer(d,1,500,250);
    vector->pointer(d,3,500,250);
    vector->pointer(d,1,500,250);
    vector->pointer(d,3,500,250);
    # Select only the canonical first anchor, not its duplicated command.
    d.edit.mask = array[3*len d.shapes[0].nodes] of {* => 0};
    d.edit.mask[0] = 1;
    require(vector->nodeoperation(d,vector->BreakNode) == vector->Ok &&
            len d.shapes[0].nodes == 5 && d.shapes[0].nodes[4].verb == vector->Cubic &&
            d.shapes[0].nodes[4].x == d.shapes[0].nodes[0].x,
            "closed cubic cut preserves all four spans as one open contour");
    require(vector->nodeoperation(d,vector->JoinNodes) == vector->Ok &&
            d.shapes[0].nodes[len d.shapes[0].nodes-1].verb == vector->Close,
            "weld open contour end anchors to close it again");
    d = nodefixture();
    vector->wireparts(d);
    builds := d.presentation.builds;
    overlay := d.presentation.overlay;
    for (iteration := 0; iteration < 3; iteration++) {
        vector->pointer(d,1,300,200);
        vector->wireparts(d);
        vector->pointer(d,3,300,200);
        vector->wireparts(d);
    }
    require(d.presentation.builds == builds && d.presentation.overlay == overlay,
            "unchanged Begin selection reuses overlay rather than reserializing it");
    d = nodefixture();
    require(vector->nodeoperation(d,vector->CuspNode) == vector->Ok &&
            d.shapes[0].joins[1] == 0, "cusp releases opposite arm");
    before = d.shapes;
    d.edit.mask = array[len d.edit.mask] of {* => 0};
    require(vector->nodeoperation(d,vector->JoinNodes) == vector->Invalid && d.shapes == before,
            "invalid node selection refuses atomically");
}


# Exercise vector selection interchange independently of the session clipboard.
# Whole figures remain immutable; replacement, append, refusals and Undo are real.
vectorclipboard()
{
    source := nodefixture();
    source.tool = vector->Select;
    copied := vector->copy(source);
    require(copied != "" && source.revision == 0,"Copy exports SVG without editing source");
    destination := vector->create();
    destination.tool = vector->Rectangle;
    destination.fill = 1;
    vector->pointer(destination,1,50,50);
    vector->pointer(destination,3,100,100);
    destination.tool = vector->Select;
    vector->pointer(destination,1,75,75);
    vector->pointer(destination,3,75,75);
    before := destination.shapes;
    require(vector->paste(destination,copied) == vector->Ok &&
            len destination.shapes == 1 && destination.shapes[0].nodes[1].verb == vector->Cubic,
            "Paste replaces selected figure at its z-order");
    require(vector->history(destination,0) && destination.shapes == before,
            "Paste is one exact Undo transaction");
    before = destination.shapes;
    require(vector->paste(destination,"broken SVG") == vector->Invalid && destination.shapes == before,
            "bad clipboard text preserves figure");
    destination.edit.selected = -1;
    require(vector->paste(destination,copied) == vector->Ok && len destination.shapes == 2,
            "Paste without selection appends and selects inserted figure");
    require(vector->remove(destination) == vector->Ok && len destination.shapes == 1,
            "whole selected figure deletion");
    require(vector->copy(source) == copied,"copy source survives destination edits");
    group := vector->create();
    group.tool = vector->Rectangle;
    group.fill = 1;
    vector->pointer(group,1,100,100);
    vector->pointer(group,3,300,300);
    group.tool = vector->Nodes;
    vector->pointer(group,1,100,100);
    vector->pointer(group,3,100,100);
    group.edit.mask[0] = group.edit.mask[3] = 1;
    require(vector->remove(group) == vector->Ok &&
            group.shapes[0].nodes[0].x == 300 && group.shapes[0].nodes[0].y == 300,
            "Delete group skips every selected leading anchor instead of promoting a deleted one");
    all := nodefixture();
    all.edit.mask = array[len all.edit.mask] of {* => 1};
    require(vector->nodeoperation(all,vector->DeleteNode) == vector->Ok && len all.shapes == 0,
            "Delete-node toolbar command removes an exhausted whole figure too");
    source = nodefixture();
    require(vector->remove(source) == vector->Ok && len source.shapes[0].nodes == 2,
            "Delete selected node through common keyboard model entry");
}



# A Pen/Brush tap previews a cubic ellipse before any path is committed.
# The private header must already describe seven-word commands at that point.
previewformat()
{
    d := vector->create();
    d.tool = vector->Pen;
    vector->defer(d,1,100,100);
    bytes := vector->wire(d);
    require(bytes[0:8] == "00020005" && len bytes == 56+32+6*56,
            "Pen dot preview advertises full cubic commands before release");
    vector->cancel(d);
    d = vector->create();
    vector->defer(d,1,100,100);
    bytes = vector->wire(d);
    require(bytes[0:8] == "00020001","Brush dot preview also uses cubic header");
    vector->cancel(d);
}
