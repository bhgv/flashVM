implement DesktopTest;
include "sys.m";
include "uibridge.m";
include "idedeskpane.m";
include "idedesktop.m";
DesktopTest: module { init: fn(); }; # Real pane processes and layout owner admission.


# Raise a named failure instead of relying on assertions disabled in release builds.
check(ok: int, message: string)
{
    if (!ok) {
        raise "fail: desktop " + message;
    }
}


# Complete one pane request; the borrowed endpoint remains live through the reply.
pane(owner: chan of Idedeskpane->Request, operation: int, event: ref Uibridge->Message): Idedeskpane->Reply
{
    replies := chan of Idedeskpane->Reply;
    owner <-= Idedeskpane->Request(1, operation, "", event, replies);
    return <-replies;
}


# Complete one geometry request, returning independently copied scene descriptors.
call(owner: chan of Idedesktop->Request, request: Idedesktop->Request): Idedesktop->Reply
{
    request.reply = chan of Idedesktop->Reply;
    owner <-= request;
    return <-request.reply;
}


# Send one copied pointer proposal against exactly the last acknowledged scene.
# owner remains borrowed; the synchronous helper consumes its unique reply.
point(owner: chan of Idedesktop->Request, ticket, x, y, phase: int): Idedesktop->Reply
{
    return call(owner, Idedesktop->Request(1, Idedesktop->Pointer, ticket, 0,
                x, y, phase, "", "", -1, -1, 0, nil));
}


# Find a copied MovieClip descriptor without assuming its current paint order.
# Return nil if the layout has removed that stable identity from this scene.
item(view: Idedesktop->Reply, id: int): ref Idedesktop->Item
{
    for (index := 0; index < len view.items; index++) {
        if (view.items[index].id == id) {
            return view.items[index];
        }
    }
    return nil;
}


# Verify independent documents, undo, readonly help, stale cancellation, blank-only
# panning and real miniature references. All owners acknowledge shutdown.
init()
{
    sys := load Sys Sys->PATH;
    panes := load Idedeskpane Idedeskpane->PATH;
    desk := load Idedesktop Idedesktop->PATH;
    codec := load Uibridge Uibridge->PATH;
    check(sys != nil && panes != nil && desk != nil && codec != nil, "modules");
    left := panes->start(panes->Editor, "", "left");
    right := panes->start(panes->Editor, "", "right");
    check(left != nil && right != nil, "owners");
    changed := pane(left, panes->Input, ref Uibridge->Message(codec->Text, 0, 0, 0, 0, 0, "π😀"));
    untouched := pane(right, panes->Snapshot, nil);
    check(changed.body == "π😀left" && changed.revision > changed.saved && untouched.body == "right" &&
          untouched.revision == untouched.saved, "independent document ownership");
    pane(left, panes->Input, ref Uibridge->Message(codec->Key, 0, 17, 1, 0, 0, ""));
    undone := pane(left, panes->Input, ref Uibridge->Message(codec->Key, 0, 90, 1, 0, 0, ""));
    check(undone.body == "left" && undone.revision > changed.revision, "real undo history");
    # Geometry is only a proposal: the document admits current-revision scalar
    # ranges, keeps a private anchor, and cancels stale/late pointer continuations.
    before := pane(right, panes->Snapshot, nil);
    selected := pane(right, panes->Place, ref Uibridge->Message(codec->Request, 0, 1, before.revision, 1, 0, ""));
    selected = pane(right, panes->Place, ref Uibridge->Message(codec->Request, 0, 2, before.revision, 4, 0, ""));
    check(selected.status == 0 && selected.caret == 4 && selected.anchor == 1 && selected.body == "right",
          "pointer selection changes no source revision");
    stale := pane(right, panes->Place, ref Uibridge->Message(codec->Request, 0, 2, before.revision - 1, 0, 0, ""));
    check(stale.status != 0 && stale.caret == 4 && stale.anchor == 1, "stale pointer preserves selection");
    late := pane(right, panes->Place, ref Uibridge->Message(codec->Request, 0, 3, before.revision, 0, 0, ""));
    check(late.status != 0 && late.caret == 4, "late end cannot revive cancelled gesture");
    selected = pane(right, panes->Place, ref Uibridge->Message(codec->Request, 0, 1, before.revision, 0, 1, ""));
    check(selected.status == 0 && selected.anchor == 1 && selected.caret == 0, "shift keeps original anchor");
    pane(right, panes->CancelPointer, nil);
    late = pane(right, panes->Place, ref Uibridge->Message(codec->Request, 0, 2, before.revision, 5, 0, ""));
    check(late.status != 0 && late.caret == 0, "explicit pointer cancellation");
    layout := desk->start();
    request := Idedesktop->Request(1, desk->Update, 0, 1, 0, 0, 0, "Left", changed.body, 0, -1, 0, nil);
    view := call(layout, request);
    check(view.status == desk->Ok, "window admission");
    mirrored := 0;
    for (index := 0; index < len view.items; index++) {
        if (view.items[index].mirror == 1) {
            mirrored++;
        }
    }
    check(mirrored == 1, "live miniature capability reference");
    request.operation = desk->TextHit;
    request.ticket = view.ticket;
    request.x = 120;
    request.y = 100;
    admitted := call(layout, request);
    check(admitted.status == desk->Ok, "layout admits only actual text body");
    request.window = 4;
    rejectedhit := call(layout, request);
    check(rejectedhit.status == desk->Conflict, "glyph metadata cannot name another pane");
    request.window = 1;
    request.ticket = view.ticket - 1;
    rejectedhit = call(layout, request);
    check(rejectedhit.status == desk->Conflict, "text hit needs current view ticket");
    request.ticket = view.ticket;
    request.y = 80;
    rejectedhit = call(layout, request);
    check(rejectedhit.status == desk->Conflict, "window header is never a source coordinate");
    request.operation = desk->Activate;
    view = call(layout, request);
    check(view.status == desk->Ok && view.scroll == 0, "body activation preserves table offset");
    request.operation = desk->Pointer;
    request.ticket = view.ticket;
    request.x = 84;
    request.y = 340;
    request.phase = 1;
    view = call(layout, request);
    request.ticket = view.ticket;
    request.y = 140;
    request.phase = 2;
    view = call(layout, request);
    check(view.scroll == 200, "empty table swipe");
    request.ticket = view.ticket - 1;
    request.y = 10;
    rejected := call(layout, request);
    check(rejected.status == desk->Conflict && rejected.scroll == 200, "stale view cancellation");
    request.operation = desk->Select;
    request.window = 1;
    view = call(layout, request);
    request.operation = desk->Pointer;
    request.ticket = view.ticket;
    request.x = 120;
    request.y = 100;
    request.phase = 1;
    view = call(layout, request);
    request.ticket = view.ticket;
    request.y = 250;
    request.phase = 2;
    view = call(layout, request);
    check(view.scroll == 0, "window gesture never pans table");
    request.operation = desk->Tool;
    request.window = desk->Debugger;
    view = call(layout, request);
    request.operation = desk->Pointer;
    request.ticket = view.ticket;
    request.x = 600;
    request.y = 330;
    request.phase = 1;
    view = call(layout, request);
    check(view.action == desk->Debugger && view.drawer == 0, "debugger panel proposal without target attachment");
    request.operation = desk->Tool;
    request.window = 0;
    view = call(layout, request);
    view = point(layout, view.ticket, 120, 80, 1);
    view = point(layout, view.ticket, 220, 180, 2);
    view = point(layout, view.ticket, 220, 180, 3);
    moved := item(view, 1);
    check(moved != nil && moved.x == 196 && moved.y == 172 && view.scroll == 0,
          "header moves a real window instead of the table");
    request.operation = desk->Update;
    request.window = 1;
    request.title = "updated";
    request.body = "still independently owned";
    request.caret = 0;
    view = call(layout, request);
    check(item(view, 1).x == 196 && item(view, 1).y == 172, "content keeps window position");
    view = point(layout, view.ticket, 220, 180, 1);
    request.operation = desk->Cancel;
    view = call(layout, request);
    view = point(layout, view.ticket, 420, 280, 2);
    check(item(view, 1).x == 196, "focus cancellation retires header drag");
    # A title close request is only a proposal. The layout never destroys a source.
    view = point(layout, view.ticket, 550, 180, 1);
    view = point(layout, view.ticket, 550, 180, 3);
    check(view.action == desk->CloseEditor && item(view, 1) != nil, "close proposes without deleting");
    request.operation = desk->Tool;
    request.window = desk->CloseEditor;
    view = call(layout, request);
    view = point(layout, view.ticket, 600, 380, 1);
    check(view.action == desk->CancelClose && view.drawer == 0, "explicit cancel control");
    request.operation = desk->Remove;
    request.window = 1;
    view = call(layout, request);
    check(view.status == desk->Ok && item(view, 1) == nil && item(view, 20) == nil &&
          item(view, 30) == nil, "remove retires window, close button and miniature together");
    request.window = 2;
    view = call(layout, request);
    check(view.status == desk->Invalid, "system pane cannot be removed");
    request.operation = desk->Update;
    request.window = 1;
    request.title = "replacement";
    request.body = "new source";
    view = call(layout, request);
    check(item(view, 1).x == 96 && item(view, 1).y == 72, "reused slot has fresh position");
    # Overlay body is opaque to hit testing, including covered header drags.
    request.operation = desk->Update;
    request.window = 2;
    request.title = "console";
    view = call(layout, request);
    request.operation = desk->Tool;
    request.window = desk->Help;
    view = call(layout, request);
    view = point(layout, view.ticket, 600, 80, 1);
    view = point(layout, view.ticket, 620, 180, 2);
    view = point(layout, view.ticket, 620, 180, 3);
    check(item(view, 2).x == 484 && item(view, 2).y == 72, "scroll blocks covered window drag");
    request.operation = desk->Tool;
    request.window = 0;
    view = call(layout, request);
    # Releasing a close press outside its own title control never closes a pane.
    view = point(layout, view.ticket, 450, 80, 1);
    view = point(layout, view.ticket, 120, 120, 3);
    check(view.action == 0 && item(view, 1) != nil, "close release outside is cancelled");
    request.operation = desk->Resize;
    request.x = 480;
    request.y = 640;
    view = call(layout, request);
    request.operation = desk->Tool;
    request.window = desk->CloseEditor;
    view = call(layout, request);
    view = point(layout, view.ticket, 100, 380, 1);
    check(view.action == desk->CancelClose, "narrow scroll uses actual button bounds");
    zoomanchor(desk);
    mdicontainers(desk);
    request.operation = desk->Shutdown;
    view = call(layout, request);
    check(view.status == desk->Ok, "layout shutdown");
    stoppedleft := pane(left, panes->Shutdown, nil);
    stoppedright := pane(right, panes->Shutdown, nil);
    check(stoppedleft.status == 0 && stoppedright.status == 0, "pane shutdown");
    sys->print("PASS desktop owners: independent edits/undo, live mirrors, guarded pan, stale rejection, shutdown\n");
}


# Exercise a scrolled/maximized pane and a genuine table zoom request. Window
# coordinates/size must remain logical values while physical translations scale.
# This independent layout owner is acknowledged before returning; no UI is mocked.
zoomanchor(desk: Idedesktop)
{
    owner := desk->start();
    request := Idedesktop->Request(1, desk->Update, 0, 5, 0, 0, 0, "Vector", "", -1, -1, 0, nil);
    view := call(owner, request);
    request.operation = desk->Select;
    view = call(owner, request);
    view = point(owner, view.ticket, 900, 385, 1);
    view = point(owner, view.ticket, 900, 385, 3);
    before := item(view, 5);
    scroll := view.scroll;
    zoom := view.zoom;
    check(scroll > 0, "zoom anchor fixture has real prior scrolling");
    view = point(owner, view.ticket, 82, 55, 5);
    after := item(view, 5);
    check(view.zoom == zoom - 10 && view.scroll == scroll * view.zoom / zoom,
          "table zoom scales physical scroll rather than changing logical anchor");
    check(after.x == before.x && after.y == before.y && after.width == before.width && after.height == before.height,
          "table zoom leaves every saved window rectangle unchanged");
    top := 48 + (after.y - 48) * view.zoom / 100 - view.scroll;
    check(top >= 47 && top <= 49, "maximized header remains below the top bar after table zoom");
    for (cycle := 0; cycle < 8; cycle++) {
        view = point(owner, view.ticket, 82, 55, 4);
        check(view.zoom == zoom && view.scroll == scroll, "opposite zoom restores anchor without cumulative integer drift");
        view = point(owner, view.ticket, 82, 55, 5);
    }
    request.operation = desk->Shutdown;
    check(call(owner, request).status == desk->Ok, "zoom fixture shutdown");
}


# Verify real presentation descriptors for homogeneous MDI parents, root motion,
# stale input and exact saved SDI geometry. Documents are not re-created here.
mdicontainers(desk: Idedesktop)
{
    owner := desk->start();
    request := Idedesktop->Request(1, desk->Update, 0, 1, 0, 0, 0, "source one", "one", -1, -1, 0, nil);
    original := call(owner, request);
    before := item(original, 1);
    request.window = 4;
    request.title = "source two";
    call(owner, request);
    request.operation = desk->SetContainer;
    request.window = 1;
    request.phase = 1;
    request.x = 1;
    grouped := call(owner, request);
    check(grouped.status == desk->Ok && item(grouped, 13) != nil && item(grouped, 1).parent == 5,
          "MDI root and actual child descriptor parent");
    stale := point(owner, original.ticket, 100, 80, 1);
    check(stale.status == desk->Conflict, "pre-parenting pointer ticket refused");
    request.window = 4;
    grouped = call(owner, request);
    check(item(grouped, 4).parent == 5 && item(grouped, 4).x > item(grouped, 1).x,
          "same-family children tile under one root");
    request.x = 2;
    refused := call(owner, request);
    check(refused.status == desk->Invalid && refused.ticket == grouped.ticket,
          "wrong-family layout assignment does not mutate membership");
    root := item(grouped, 13);
    grabbed := point(owner, grouped.ticket, root.x + 10, root.y + 10, 1);
    moved := point(owner, grabbed.ticket, root.x + 45, root.y + 40, 2);
    moved = point(owner, moved.ticket, root.x + 45, root.y + 40, 3);
    check(item(moved, 13).x == root.x + 35 && item(moved, 13).y == root.y + 30 &&
          item(moved, 1).x == item(grouped, 1).x && item(moved, 1).y == item(grouped, 1).y,
          "moving a container keeps child local geometry unchanged");
    root = item(moved, 13);
    grabbed = point(owner, moved.ticket, root.x + 10, root.y + 10, 1);
    request.operation = desk->Resize;
    request.x = 960;
    request.y = 640;
    resized := call(owner, request);
    cancelled := point(owner, resized.ticket, root.x + 55, root.y + 45, 2);
    check(item(cancelled, 13).x == root.x && item(cancelled, 13).y == root.y,
          "resize cancels an in-progress container drag");
    root = item(cancelled, 13);
    ignoredzoom := point(owner, cancelled.ticket, root.x + 18, root.y + 50, 4);
    check(ignoredzoom.status == desk->Ok, "fixed MDI child zoom does not corrupt saved SDI state");
    request.operation = desk->SetContainer;
    request.window = 1;
    request.x = 1;
    request.phase = 0;
    detached := call(owner, request);
    after := item(detached, 1);
    check(after.parent == 1 && after.x == before.x && after.y == before.y &&
          after.width == before.width && after.height == before.height && item(detached, 4).parent == 5,
          "detach restores saved SDI geometry without detaching siblings");
    request.window = 4;
    empty := call(owner, request);
    check(item(empty, 13) == nil && item(empty, 4).parent == 1, "empty MDI root retired");
    request.operation = desk->Update;
    request.window = 5;
    request.title = "source three";
    call(owner, request);
    slots := array[] of {1, 4, 5};
    for (index := 0; index < len slots; index++) {
        request.operation = desk->SetContainer;
        request.window = slots[index];
        request.x = 1;
        request.phase = 8;
        call(owner, request);
    }
    request.operation = desk->Select;
    request.window = 1;
    revealed := call(owner, request);
    root = item(revealed, 28);
    bottom := 48 + (root.y - 48 + root.height) * revealed.zoom / 100 - revealed.scroll;
    check(bottom <= 635, "last container identity reveals its complete outer frame, not just child bounds");
    request.operation = desk->Shutdown;
    check(call(owner, request).status == desk->Ok, "MDI layout shutdown acknowledgement");
}
