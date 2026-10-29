implement BuildpanelTest;
include "sys.m";
include "idecore.m";
include "ideworkspace.m";
include "fircompiler.m";
include "idecompiler.m";
include "idebuildpanel.m";
BuildpanelTest: module
{
    init: fn(); # Exercise real compilation and revision-bound editor admission.
};


# Raise a specific failure; no disabled assertions or placeholder success paths.
# condition/message are borrowed values; returns only when the condition holds.
check(condition: int, message: string)
{
    if (!condition) {
        raise "fail: build panel " + message;
    }
}


# Perform one actual editor rendezvous and return its copied reply.
# document/request are borrowed; only this calling process blocks.
doccall(document: chan of Idecore->Request, request: Idecore->Request): Idecore->Reply
{
    request.reply = chan of Idecore->Reply;
    document <-= request;
    return <-request.reply;
}


# Perform one panel rendezvous; caller consumes every reply including shutdown.
# endpoint/request are borrowed, and mutable outputs belong to this caller.
call(endpoint: chan of Idebuildpanel->Request, request: Idebuildpanel->Request): Idebuildpanel->Reply
{
    request.reply = chan of Idebuildpanel->Reply;
    endpoint <-= request;
    return <-request.reply;
}


# Forward real document requests while another editor deterministically changes
# source before the second Snapshot. End publication therefore sees a newer
# revision after real native compilation, without timing assumptions/test hooks.
# document is borrowed; requests is owned until an acknowledged Shutdown relay.
competingeditor(document: chan of Idecore->Request, requests: chan of Idecore->Request)
{
    snapshots := 0;
    for (;;) {
        request := <-requests;
        if (request.operation == Idecore->Snapshot) {
            snapshots++;
            if (snapshots == 2) {
                current := doccall(document, Idecore->Request(1, Idecore->Snapshot, 0, 0, 0, "", "", nil));
                edited := doccall(document, Idecore->Request(2, Idecore->Replace,
                                  current.revision, 0, 0, "# concurrent edit\n", "", nil));
                check(edited.status == Idecore->Ok, "competing editor edit");
            }
        }
        response := doccall(document, request);
        request.reply <-= response;
        if (request.operation == Idecore->Shutdown) {
            return;
        }
    }
}


# Verify binary copying, all three targets, original UTF-8 diagnostics, invalid
# options, stale requests/results and owner shutdown in the actual Dis engine.
# No parameters; all channel/module roots remain live until acknowledgements.
init()
{
    sys := load Sys Sys->PATH;
    core := load Idecore Idecore->PATH;
    workspace := load Ideworkspace Ideworkspace->PATH;
    compiler := load Idecompiler Idecompiler->PATH;
    panel := load Idebuildpanel Idebuildpanel->PATH;
    check(sys != nil && core != nil && workspace != nil && compiler != nil && panel != nil, "modules");
    document := core->start();
    source := "# π😀 original\nimplement M;M:module{value:fn():int;};value():int{return 6*7;}";
    opened := doccall(document, Idecore->Request(1, core->Open, 0, 0, 0, source, "", nil));
    endpoint := panel->start(core, document, workspace, compiler);
    check(endpoint != nil && opened.status == core->Ok, "start");
    job := Idebuildpanel->Request(42, panel->Artifact, opened.revision, 0, 0, 0, nil);
    result := call(endpoint, job);
    check(result.id == 42 && result.status == panel->NoArtifact && result.artifact == nil, "no artifact");
    for (target := Fircompiler->Dis; target <= Fircompiler->Swf; target++) {
        job.operation = panel->Build;
        job.language = Fircompiler->Limbo;
        job.target = target;
        job.optimize = 1;
        result = call(endpoint, job);
        check(result.status == panel->Ok && result.compiler == Fircompiler->Ok &&
              result.revision == opened.revision && result.target == target && result.artifact == nil, "build target status=" + string result.status + " native=" + string result.compiler + " diagnostic=" + result.diagnostic);
        job.operation = panel->Artifact;
        artifact := call(endpoint, job);
        check(artifact.status == panel->Ok && artifact.target == target && artifact.language == Fircompiler->Limbo &&
              len artifact.artifact > 0 && len artifact.debug > 0, "fetch target");
        first := artifact.artifact[0];
        debugfirst := artifact.debug[0];
        artifact.artifact[0] = byte 255;
        artifact.debug[0] = byte 255;
        copied := call(endpoint, job);
        check(copied.artifact[0] == first && copied.debug[0] == debugfirst, "independent arrays");
    }
    job.operation = panel->Build;
    job.language = 99;
    result = call(endpoint, job);
    check(result.status == panel->Invalid && result.compiler == -1, "invalid option");
    job.language = Fircompiler->AS2;
    result = call(endpoint, job);
    check(result.status == panel->BuildFailed && result.compiler == Fircompiler->Source, "failed cross-language build");
    job.operation = panel->Artifact;
    result = call(endpoint, job);
    check(result.status == panel->Ok && result.language == Fircompiler->Limbo && result.target == Fircompiler->Swf,
          "failure preserves last success metadata");
    bad := "# π😀 diagnostic\nimplement M;M:module{value:fn():int;};value():int{return missing;}";
    changed := doccall(document, Idecore->Request(2, core->Replace, opened.revision, 0, len source, bad, "", nil));
    result = call(endpoint, job);
    check(result.status == panel->Conflict && result.artifact == nil, "old caller revision");
    job.revision = changed.revision;
    result = call(endpoint, job);
    check(result.status == panel->NoArtifact && result.artifact == nil, "edited artifact invalidation");
    job.operation = panel->Diagnostic;
    result = call(endpoint, job);
    check(result.status == panel->NoDiagnostic, "edited diagnostic invalidation");
    job.operation = panel->Build;
    job.language = Fircompiler->Limbo;
    job.target = Fircompiler->Dis;
    result = call(endpoint, job);
    check(result.status == panel->BuildFailed && result.compiler == Fircompiler->Source, "source failure");
    job.operation = panel->Diagnostic;
    result = call(endpoint, job);
    check(result.status == panel->Ok && result.document.status == core->Ok &&
          result.document.text[result.document.offset:result.document.selection_end] == "missing" &&
          result.document.text == bad && result.document.saved == opened.saved, "Unicode span without edits");
    job.operation = panel->Shutdown;
    result = call(endpoint, job);
    check(result.status == panel->Ok, "shutdown");
    restored := doccall(document, Idecore->Request(3, core->Replace, changed.revision, 0, len bad, source, "", nil));
    proxy := chan of Idecore->Request;
    spawn competingeditor(document, proxy);
    endpoint = panel->start(core, proxy, workspace, compiler);
    check(endpoint != nil, "race start");
    job.operation = panel->Build;
    job.revision = restored.revision;
    result = call(endpoint, job);
    check(result.status == panel->Conflict && result.compiler == Fircompiler->Ok && result.artifact == nil,
          "successful stale native result rejected");
    job.operation = panel->Artifact;
    job.revision = result.document.revision;
    result = call(endpoint, job);
    check(result.status == panel->NoArtifact, "stale result never retained");
    job.operation = panel->Shutdown;
    result = call(endpoint, job);
    check(result.status == panel->Ok, "race shutdown");
    stopped := doccall(proxy, Idecore->Request(4, core->Shutdown, result.document.revision, 0, 0, "", "", nil));
    check(stopped.status == core->Ok, "borrowed editor remains live until explicit stop");
    sys->print("PASS compiler panel: targets=3 copies=ok UTF8=ok stale-native=reject shutdown=ok\n");
}
