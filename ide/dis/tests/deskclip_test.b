implement DeskclipTest;
include "sys.m";
include "uibridge.m";
include "idedeskpane.m";
include "idedeskclip.m";
DeskclipTest: module
{
    init: fn(); # Exercise byte limits, stale selection and source preservation.
};


# Fail explicitly if one public contract is violated. ok/name are copied values;
# no state is changed on success and no assertion evaluation can be compiled out.
check(ok: int, name: string)
{
    if (!ok) {
        raise "fail: session clipboard " + name;
    }
}


# Consume one pane acknowledgement. owner is borrowed; text/event are immutable
# requests. Return a copied snapshot; never reuse owner after terminal Shutdown.
pane(owner: chan of Idedeskpane->Request, operation: int, text: string,
     event: ref Uibridge->Message): Idedeskpane->Reply
{
    replies := chan of Idedeskpane->Reply;
    owner <-= Idedeskpane->Request(1, operation, text, event, replies);
    return <-replies;
}


# Complete one session-value request with a private reply endpoint. The expected
# generation guards Put/Clear; rejected requests return the unchanged snapshot.
clip(owner: chan of Idedeskclip->Request, operation, expected: int, text: string): Idedeskclip->Reply
{
    replies := chan of Idedeskclip->Reply;
    owner <-= Idedeskclip->Request(1, operation, expected, text, replies);
    return <-replies;
}


# Run real independent services; compare complete values and revisions. No files,
# native clipboard, graphics or compiler are used. Shutdown consumes every owner.
init()
{
    sys := load Sys Sys->PATH;
    clips := load Idedeskclip Idedeskclip->PATH;
    panes := load Idedeskpane Idedeskpane->PATH;
    check(sys != nil && clips != nil && panes != nil, "dependencies");
    board := clips->start();
    initial := clip(board, clips->Get, 0, "");
    check(initial.status == clips->Ok && initial.bytes == 0 && initial.generation == 0, "initial value");
    block := "x";
    while (len block < clips->MaxBytes) {
        block += block;
    }
    stored := clip(board, clips->Put, initial.generation, block);
    check(stored.status == clips->Ok && stored.bytes == clips->MaxBytes && stored.text == block, "exact byte bound");
    rejected := clip(board, clips->Put, stored.generation, block + "x");
    check(rejected.status == clips->TooLarge && rejected.text == block, "oversize preserves value");
    wide := "π";
    while (len wide < clips->MaxBytes) {
        wide += wide;
    }
    rejected = clip(board, clips->Put, stored.generation, wide);
    check(rejected.status == clips->TooLarge && rejected.generation == stored.generation,
          "byte bound is not a Unicode character bound");
    rejected = clip(board, clips->Clear, initial.generation, "");
    check(rejected.status == clips->Conflict && rejected.text == block, "stale clear preserves value");
    cleared := clip(board, clips->Clear, stored.generation, "");
    check(cleared.status == clips->Ok && cleared.text == "" && stored.text == block, "retained replies are immutable");
    large := panes->start(panes->Editor, "", wide);
    check(large != nil, "large Unicode source owner");
    pane(large, panes->Input, "", ref Uibridge->Message(Uibridge->Key, 0, 17, 1, 0, 0, ""));
    selected := pane(large, panes->Input, "", ref Uibridge->Message(Uibridge->Key, 0, 65, 1, 0, 0, ""));
    exported := pane(large, panes->ExportSelection, "", nil);
    check(exported.status != 0 && exported.transfer == "" && exported.revision == selected.revision &&
          exported.rangelast == len wide, "oversize export preserves full source selection");
    pane(large, panes->Shutdown, "", nil);
    editor := panes->start(panes->Editor, "", "abcdef");
    check(editor != nil, "source owner");
    current := pane(editor, panes->Snapshot, "", nil);
    pane(editor, panes->Place, "", ref Uibridge->Message(Uibridge->Request, 0, 1, current.revision, 1, 0, ""));
    pane(editor, panes->Place, "", ref Uibridge->Message(Uibridge->Request, 0, 3, current.revision, 4, 0, ""));
    old := pane(editor, panes->ExportSelection, "", nil);
    pane(editor, panes->Place, "", ref Uibridge->Message(Uibridge->Request, 0, 1, current.revision, 2, 0, ""));
    pane(editor, panes->Place, "", ref Uibridge->Message(Uibridge->Request, 0, 3, current.revision, 3, 0, ""));
    stale := pane(editor, panes->CutSelection, "", ref Uibridge->Message(Uibridge->Request, 0, 0,
                  old.revision, old.rangefirst, old.rangelast, ""));
    check(stale.status != 0 && stale.body == "abcdef" && stale.revision == old.revision,
          "same-revision changed selection refuses cut");
    empty := pane(editor, panes->PasteSelection, "", ref Uibridge->Message(Uibridge->Request, 0, 0,
                  stale.revision, stale.rangefirst, stale.rangelast, ""));
    check(empty.status == 0 && empty.revision == stale.revision && empty.rangefirst == 2 && empty.rangelast == 3,
          "empty paste neither deletes nor collapses selection");
    invalid := pane(editor, panes->PasteSelection, "bad\0text", ref Uibridge->Message(Uibridge->Request, 0, 0,
                    empty.revision, empty.rangefirst, empty.rangelast, ""));
    check(invalid.status != 0 && invalid.body == "abcdef" && invalid.revision == empty.revision,
          "direct client cannot bypass text validation");
    pane(editor, panes->Shutdown, "", nil);
    stopped := clip(board, clips->Shutdown, 0, "");
    check(stopped.status == clips->Ok && stopped.text == "", "terminal shutdown clears retained text");
    sys->print("PASS desktop clipboard guards: exact bytes, stale generation/range, full source, immutable values, empty/invalid paste\n");
}
