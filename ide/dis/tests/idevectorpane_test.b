implement VectorPaneTest;
include "sys.m";
include "uibridge.m";
include "idecore.m";
include "idedeskpane.m";
sys: Sys; # Explicit test failure and output capability.
panes: Idedeskpane; # Real channel-owning implementation.


# No GUI is required to prove independent ownership and revision admission.
VectorPaneTest: module {
    init: fn();
};


# Fail explicitly in every build; the host also rejects missing PASS/FAIL output.
require(ok: int, label: string)
{
    if (!ok) {
        sys->print("FAIL vector pane: %s\n", label);
        control := sys->open("/dev/sysctl", Sys->OWRITE);
        data := array of byte "halt";
        if (control != nil) {
            sys->write(control, data, len data);
        }
        raise "fail: vector pane";
    }
}


# Complete one owning-channel rendezvous. All data is copied and no endpoint escapes.
call(owner: chan of Idedeskpane->Request, op: int, event: ref Uibridge->Message): Idedeskpane->Reply
{
    reply := chan of Idedeskpane->Reply;
    owner <-= Idedeskpane->Request(1, op, "", event, reply);
    return <-reply;
}


# Check two actual spawned vector controllers, stale drafts and acknowledged shutdown.
init()
{
    sys = load Sys Sys->PATH;
    panes = load Idedeskpane Idedeskpane->PATH;
    a := panes->start(panes->Vector, "", "");
    b := panes->start(panes->Vector, "", "");
    require(a != nil && b != nil, "independent startup");
    empty := call(b, panes->Snapshot, nil);
    require(empty.revision == 0 && empty.saved == 0 && empty.drawing == "00000000", "clean serialized vector");
    first := call(a, panes->Draw, ref Uibridge->Message(Uibridge->Request, 0, 1, 0, 100, 100, ""));
    require(first.status == 0 && first.revision == 1 && first.saved == 0, "uncommitted preview is dirty");
    stale := call(a, panes->Draw, ref Uibridge->Message(Uibridge->Request, 0, 2, 0, 200, 200, ""));
    require(stale.status == Idecore->Conflict && stale.drawing == first.drawing, "stale proposal preserves preview");
    require(call(b, panes->Snapshot, nil).drawing == empty.drawing, "other vector owner unchanged");
    first = call(a, panes->Draw, ref Uibridge->Message(Uibridge->Request, 0, 3, 1, 300, 200, ""));
    require(first.status == 0 && first.revision == 1 && first.drawing != empty.drawing, "one committed stroke");
    refused := call(a, panes->Open, nil);
    require(refused.status != 0 && refused.drawing == first.drawing, "dirty Open refusal atomic");
    refused = call(a, panes->Save, nil);
    require(refused.status != 0 && refused.saved == 0 && refused.drawing == first.drawing, "pathless Save cannot mark saved");
    call(a, panes->Input, ref Uibridge->Message(Uibridge->Key, 0, 17, 1, 0, 0, ""));
    undone := call(a, panes->Input, ref Uibridge->Message(Uibridge->Key, 0, 90, 1, 0, 0, ""));
    require(undone.drawing == empty.drawing && undone.revision == 2, "Undo through ordinary key channel");
    call(a, panes->Input, ref Uibridge->Message(Uibridge->Key, 0, 17, 0, 0, 0, ""));
    changed := call(a, panes->Input, ref Uibridge->Message(Uibridge->Text, 0, 0, 0, 0, 0, "r2f+"));
    require(changed.body == "B/R/E/L | 1..6 color | F fill\nRectangle #e04444 w=9 f=1 | +/- Z/Y" &&
            changed.revision == 2, "batched SDL text changes style without source revision");
    malformed := call(a, panes->Draw, ref Uibridge->Message(Uibridge->Text, 0, 1, 2, 100, 100, ""));
    require(malformed.status == Idecore->Invalid && malformed.drawing == empty.drawing,
            "malformed draw uses shared pane Invalid status and keeps source");
    require(call(a, panes->Shutdown, nil).status == 0 && call(b, panes->Shutdown, nil).status == 0,
            "independent acknowledged shutdown");
    sys->print("PASS vector panes: revision, dirty file refusal, isolated owners and shutdown\n");
}
