implement Nativeinktest;
include "sys.m";
include "uibridge.m";
include "idedeskpane.m";
Nativeinktest: module { init: fn(); }; # Real C buffers, pane/worker, SVG and history.


# Keep failures active in interpreter and native JIT release dispatch.
check(ok: int, label: string)
{
    if (!ok) {
        raise "fail: native ink " + label;
    }
}


# Send one copied request and wait only for its owner acknowledgement.
call(owner: chan of Idedeskpane->Request, operation: int, text: string,
     event: ref Uibridge->Message): Idedeskpane->Reply
{
    reply := chan of Idedeskpane->Reply;
    owner <-= Idedeskpane->Request(1, operation, text, event, reply);
    return <-reply;
}


# Queue both native frozen strokes before joining the worker with Save. A queued
# job must carry earlier geometry/history even when its owner's snapshot predates
# that completion. Real SVG output is independently inspected by the host test.
init()
{
    sys := load Sys Sys->PATH;
    panes := load Idedeskpane Idedeskpane->PATH;
    check(sys != nil && panes != nil, "modules");
    notices := chan of int;
    owner := panes->startnotified(panes->Vector, "", "", notices, 1);
    check(owner != nil, "owner");
    for (stroke := 0; stroke < 2; stroke++) {
        x := 100;
        y := 100;
        if (stroke != 0) {
            x = 500;
            y = 400;
        }
        if (stroke != 0) {
            changed := call(owner, panes->VectorControl, "", ref Uibridge->Message(
                Uibridge->Request, 0, 13, stroke, 0, 0, ""));
            check(changed.status == 0, "color accepted while earlier ink is queued");
            check(changed.body != "", "color response is available before Save joins workers");
        }
        token := "00000001";
        if (stroke != 0) {
            token = "00000002";
        }
        # Real native Begin carries its token and must not join the previous fit.
        begun := call(owner, panes->Draw, "", ref Uibridge->Message(Uibridge->Request, 0, 1, stroke, x, y, token));
        check(begun.status == 0 && begun.revision == stroke + 1, "begin metadata");
        ended := call(owner, panes->Draw, "", ref Uibridge->Message(Uibridge->Request, 0, 3,
            begun.revision, x + 96, y + 24, token));
        check(ended.status == 0 && ended.revision == stroke + 1, "frozen-token admission");
    }
    saved := call(owner, panes->Save, "native-ink.svg", nil);
    check(saved.status == 0 && saved.revision == 2, "ordered worker save");
    undone := call(owner, panes->VectorControl, "", ref Uibridge->Message(Uibridge->Request, 0, 5, 2, 0, 0, ""));
    check(undone.status == 0 && undone.revision == 3, "whole-stroke undo");
    check(call(owner, panes->Save, "native-undo.svg", nil).status == 0, "undo save");
    redone := call(owner, panes->VectorControl, "", ref Uibridge->Message(Uibridge->Request, 0, 6, 3, 0, 0, ""));
    check(redone.status == 0 && redone.revision == 4, "whole-stroke redo");
    check(call(owner, panes->Save, "native-redo.svg", nil).status == 0, "redo save");
    begun := call(owner, panes->Draw, "", ref Uibridge->Message(Uibridge->Request, 0, 1, 4, 800, 500, ""));
    check(begun.status == 0, "negative-control begin");
    rejected := call(owner, panes->Draw, "", ref Uibridge->Message(Uibridge->Request, 0, 3, 5, 820, 520, "000003ff"));
    check(rejected.status == 0, "invalid frozen token is processed off the UI owner");
    check(call(owner, panes->Save, "native-invalid.svg", nil).status != 0,
          "failed native acquisition cannot publish an incomplete document");
    check(call(owner, panes->Shutdown, "", nil).status == 0, "worker shutdown");
    sys->print("PASS native frozen ink: ordered Limbo worker, SVG, undo and redo\n");
}
