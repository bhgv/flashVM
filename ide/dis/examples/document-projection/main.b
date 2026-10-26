# Real document projection regression: no synthetic target or native document access.
implement Docprojectioncheck;
include "sys.m";
include "idecore.m";
include "idevalue.m";
include "ideprojection.m";
include "idedocprojection.m";


Docprojectioncheck: module
{
    init: fn(); # Native test entry.
};


# Exchange one direct editor operation to establish/check actual owner state.
# document is borrowed; fields are copied; returns the real owner's full acknowledgement.
doc(document: chan of Idecore->Request, operation, expected, length: int, text: string): Idecore->Reply
{
    reply := chan of Idecore->Reply;
    document <-= Idecore->Request(1, operation, expected, 0, length, text, "", reply);
    return <-reply;
}


# Exchange one copied projection request; consumes exactly one result and checks origin correlation.
# endpoint is borrowed. number carries expected revision for this adapter's mutation operations.
project(endpoint: chan of ref Ideprojection->Request, operation: int, member: string,
        target, expected, permit: int, value: ref Idevalue->Value): Ideprojection->Reply
{
    reply := chan of Ideprojection->Reply;
    endpoint <-= ref Ideprojection->Request(41, 7, operation, member, expected, "", permit, 23,
                                          target, value, reply);
    result := <-reply;
    if (result.id != 41 || result.site != 23 || result.epoch != 7) {
        raise "fail: document projection correlation";
    }
    return result;
}


# Test copied reads, bounded snapshots, exact revision writes, real bound history and lifecycle.
# The ordinary editor channel remains authoritative throughout adapter reads/mutations.
init()
{
    s := load Sys Sys->PATH;
    c := load Idecore Idecore->PATH;
    v := load Idevalue Idevalue->PATH;
    p := load Idedocprojection Idedocprojection->PATH;
    if (s == nil || c == nil || v == nil || p == nil) {
        raise "fail: document projection modules";
    }
    document := c->start();
    state := doc(document, c->Open, 0, 0, "Я😀 first");
    endpoint := p->start(c, v, document, 7, "editor-document");
    result := project(endpoint, Ideprojection->ReadAs, "text", Idevalue->Text, 0, 0, nil);
    if (result.status != Ideprojection->Ok || result.value == nil || result.value.text != state.text) {
        raise "fail: real document text projection";
    }
    result.value.text = "sender mutation";
    snapshot := project(endpoint, Ideprojection->ToTable, "", Idevalue->Table, 0, 0, nil);
    if (snapshot.status != Ideprojection->Ok || snapshot.value == nil || len snapshot.value.names != 5
        || snapshot.value.values[0].integer != big 8 || snapshot.value.values[4].owner != "editor-document") {
        raise "fail: document table snapshot";
    }
    saved := state.revision;
    state = doc(document, c->Replace, state.revision, len state.text, "second");
    undo := project(endpoint, Ideprojection->ReadAs, "undo", Idevalue->Method, 0, 0, nil);
    if (undo.value == nil || undo.value.member != "undo") {
        raise "fail: live bound history method";
    }
    stale := project(endpoint, Ideprojection->InvokeRef, "", 0, saved, 1, undo.value);
    if (stale.status != Ideprojection->Stale || stale.revision != state.revision) {
        raise "fail: stale method changed document";
    }
    denied := project(endpoint, Ideprojection->InvokeRef, "", 0, state.revision, 0, undo.value);
    if (denied.status != Ideprojection->Denied) {
        raise "fail: implicit history execution";
    }
    undone := project(endpoint, Ideprojection->InvokeRef, "", 0, state.revision, 1, undo.value);
    state = doc(document, c->Snapshot, 0, 0, "");
    if (undone.status != Ideprojection->Ok || state.text != "Я😀 first" || state.revision != undone.revision) {
        raise "fail: projected undo did not reach actual history owner";
    }
    text := ref Idevalue->Value(Idevalue->Text, big 0, 0.0, "third 😀", "", 0, "", nil, nil);
    written := project(endpoint, Ideprojection->WriteValue, "text", 0, state.revision, 0, text);
    text.text = "mutated after acknowledgement";
    state = doc(document, c->Snapshot, 0, 0, "");
    if (written.status != Ideprojection->Ok || state.text != "third 😀") {
        raise "fail: projected typed write alias or no real mutation";
    }
    pause := project(endpoint, Ideprojection->Pause, "", 0, 0, 0, nil);
    blocked := project(endpoint, Ideprojection->InvokeRef, "", 0, state.revision, 1, undo.value);
    if (pause.status != Ideprojection->Ok || blocked.status != Ideprojection->Paused) {
        raise "fail: adapter cooperative mutation pause";
    }
    reading := project(endpoint, Ideprojection->ReadAs, "length", Idevalue->Big, 0, 0, nil);
    if (reading.value == nil || reading.value.integer != big 7) {
        raise "fail: paused metadata projection";
    }
    resumed := project(endpoint, Ideprojection->Resume, "", 0, 0, 0, nil);
    # Metadata remains available for normal editor documents beyond the scalar copy quota.
    large := "";
    for (i := 0; i < 5000; i++) {
        large += "x";
    }
    state = doc(document, c->Replace, state.revision, len state.text, large);
    oversized := project(endpoint, Ideprojection->ReadAs, "text", Idevalue->Text, 0, 0, nil);
    metadata := project(endpoint, Ideprojection->ReadAs, "length", Idevalue->Big, 0, 0, nil);
    if (oversized.status != Ideprojection->Invalid || oversized.value != nil
        || metadata.value == nil || metadata.value.integer != big 5000) {
        raise "fail: large document projection quota/metadata";
    }
    # Release retires only the adapter; ordinary document operations must remain alive.
    released := project(endpoint, Ideprojection->Release, "", 0, 0, 0, nil);
    state = doc(document, c->Replace, state.revision, len state.text, "still alive");
    if (released.status != Ideprojection->Ok || state.status != c->Ok) {
        raise "fail: adapter release stopped document owner";
    }
    state = doc(document, c->Shutdown, state.revision, 0, "");
    s->print("PASS live document projections: Unicode metadata snapshots stale guards bound undo typed write pause release\n");
}
