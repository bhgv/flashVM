# Prove an OFF host keeps the worker honest and balances the workspace owner.
implement CompilerUnavailable;
include "sys.m";
include "idecore.m";
include "ideworkspace.m";
include "fircompiler.m";
include "idecompiler.m";
include "idebuildpanel.m";

CompilerUnavailable: module
{
    init: fn(); # Verify missing capability on the actual interpreter/JIT host.
};


# Load portable modules but require the optional native builtin to be absent.
# No arguments; consume the workspace's terminal acknowledgement before success.
init()
{
    sys := load Sys Sys->PATH;
    core := load Idecore Idecore->PATH;
    workspace := load Ideworkspace Ideworkspace->PATH;
    worker := load Idecompiler Idecompiler->PATH;
    native := load Fircompiler Fircompiler->PATH;
    if (sys == nil || core == nil || workspace == nil || worker == nil || native != nil) {
        raise "fail: unavailable compiler capability";
    }
    panel := load Idebuildpanel Idebuildpanel->PATH;
    document := core->start();
    if (panel == nil || panel->start(core, document, workspace, worker) != nil) {
        raise "fail: unavailable compiler panel started";
    }
    documents := chan of Idecore->Reply;
    document <-= Idecore->Request(1, core->Shutdown, 0, 0, 0, "", "", documents);
    retired := <-documents;
    if (retired.status != core->Ok) {
        raise "fail: unavailable compiler panel borrowed editor shutdown";
    }
    owner := workspace->start(core);
    if (worker->start(workspace, owner) != nil) {
        raise "fail: unavailable compiler worker started";
    }
    reply := chan of Ideworkspace->Reply;
    command := Idecore->Request(1, Idecore->Snapshot, 0, 0, 0, "", "", nil);
    owner <-= Ideworkspace->Request(1, Ideworkspace->Shutdown, 0, "", "", command, 0, 0, "", 0, nil, 1, reply);
    stopped := <-reply;
    if (stopped.status != Ideworkspace->Ok) {
        raise "fail: unavailable compiler workspace shutdown";
    }
    sys->print("PASS compiler unavailable: no native fallback, worker=nil, shutdown=ok\n");
}
