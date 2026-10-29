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
    layout := desk->start();
    request := Idedesktop->Request(1, desk->Update, 0, 1, 0, 0, 0, "Left", changed.body, 0, nil);
    view := call(layout, request);
    check(view.status == desk->Ok, "window admission");
    mirrored := 0;
    for (index := 0; index < len view.items; index++) {
        if (view.items[index].mirror == 1) {
            mirrored++;
        }
    }
    check(mirrored == 1, "live miniature capability reference");
    request.operation = desk->Pointer;
    request.ticket = view.ticket;
    request.x = 84; request.y = 340; request.phase = 1;
    view = call(layout, request);
    request.ticket = view.ticket;
    request.y = 140; request.phase = 2;
    view = call(layout, request);
    check(view.scroll == 200, "empty table swipe");
    request.ticket = view.ticket - 1;
    request.y = 10;
    rejected := call(layout, request);
    check(rejected.status == desk->Conflict && rejected.scroll == 200, "stale view cancellation");
    request.operation = desk->Select; request.window = 1;
    view = call(layout, request);
    request.operation = desk->Pointer; request.ticket = view.ticket;
    request.x = 120; request.y = 100; request.phase = 1;
    view = call(layout, request);
    request.ticket = view.ticket; request.y = 250; request.phase = 2;
    view = call(layout, request);
    check(view.scroll == 0, "window gesture never pans table");
    request.operation = desk->Tool; request.window = desk->Debugger;
    view = call(layout, request);
    request.operation = desk->Pointer; request.ticket = view.ticket;
    request.x = 600; request.y = 330; request.phase = 1;
    view = call(layout, request);
    check(view.action == 0 && view.drawer == desk->Debugger, "no fabricated debugger session");
    request.operation = desk->Shutdown;
    view = call(layout, request);
    check(view.status == desk->Ok, "layout shutdown");
    stoppedleft := pane(left, panes->Shutdown, nil);
    stoppedright := pane(right, panes->Shutdown, nil);
    check(stoppedleft.status == 0 && stoppedright.status == 0, "pane shutdown");
    sys->print("PASS desktop owners: independent edits/undo, live mirrors, guarded pan, stale rejection, shutdown\n");
}
