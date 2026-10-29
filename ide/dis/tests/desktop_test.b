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
                x, y, phase, "", "", -1, -1, nil));
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
    request := Idedesktop->Request(1, desk->Update, 0, 1, 0, 0, 0, "Left", changed.body, 0, -1, nil);
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
    check(view.action == 0 && view.drawer == desk->Debugger, "no fabricated debugger session");
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
    request.operation = desk->Shutdown;
    view = call(layout, request);
    check(view.status == desk->Ok, "layout shutdown");
    stoppedleft := pane(left, panes->Shutdown, nil);
    stoppedright := pane(right, panes->Shutdown, nil);
    check(stoppedleft.status == 0 && stoppedright.status == 0, "pane shutdown");
    sys->print("PASS desktop owners: independent edits/undo, live mirrors, guarded pan, stale rejection, shutdown\n");
}
