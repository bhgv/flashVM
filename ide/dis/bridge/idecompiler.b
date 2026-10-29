implement Idecompiler;
include "idecore.m";
include "ideworkspace.m";
include "fircompiler.m";
include "idecompiler.m";


# Send a copied request to the borrowed workspace endpoint and consume its reply.
# Returns the workspace result; no response channel or document authority escapes.
call(owner: chan of Ideworkspace->Request, request: Ideworkspace->Request): Ideworkspace->Reply
{
    request.reply = chan of Ideworkspace->Reply;
    owner <-= request;
    return <-request.reply;
}


# Serve one FIFO on this Dis process. workspace/native/owner stay rooted until
# acknowledged shutdown; requests/results are copied values. Native compilation
# blocks this VM owner, but ticket/revision admission remains the workspace's job.
serve(workspace: Ideworkspace, native: Fircompiler,
      owner: chan of Ideworkspace->Request, requests: chan of Request)
{
    command := Idecore->Request(0, Idecore->Snapshot, 0, 0, 0, "", "", nil);
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        response := Reply(request.id, Ideworkspace->Invalid, -1, 0, "", nil, 0, 0, -1, 0);
        if (request.id < 0) {
            request.reply <-= response;
            continue;
        }
        if (request.operation == Shutdown) {
            response.status = Ideworkspace->Ok;
            request.reply <-= response;
            return;
        }
        if (request.operation != Build || request.language < Fircompiler->AS1 ||
            request.language > Fircompiler->Limbo || request.target < Fircompiler->Dis ||
            request.target > Fircompiler->Swf || (request.optimize != 0 && request.optimize != 1)) {
            request.reply <-= response;
            continue;
        }
        target := "dis";
        if (request.target == Fircompiler->Fir) {
            target = "fir";
        } else if (request.target == Fircompiler->Swf) {
            target = "swf";
        }
        job := Ideworkspace->Request(request.id, Ideworkspace->BeginBuild, request.document,
                                     "", "", command, request.revision, 0, target, 0, nil, 0, nil);
        snapshot := call(owner, job);
        response.status = snapshot.status;
        if (snapshot.status != Ideworkspace->Ok) {
            request.reply <-= response;
            continue;
        }
        response.ticket = snapshot.ticket;
        artifact: array of byte;
        debug: array of byte;
        (response.compiler, artifact, debug, response.diagnostic, response.kind,
         response.line, response.offset, response.length) =
            native->compile(request.language, request.target, request.optimize, array of byte snapshot.text);
        job.operation = Ideworkspace->FinishBuild;
        job.ticket = snapshot.ticket;
        job.success = response.compiler == Fircompiler->Ok;
        job.artifact = artifact;
        job.text = response.diagnostic;
        finished := call(owner, job);
        response.status = finished.status;
        if (finished.status == Ideworkspace->Ok && response.compiler == Fircompiler->Ok) {
            response.debug = debug;
        }
        request.reply <-= response;
    }
}


# Load the exact optional native module and retain it on the spawned server stack.
# Return a fresh channel or nil; no subprocess, UI call or workspace ownership transfer.
start(workspace: Ideworkspace, owner: chan of Ideworkspace->Request): chan of Request
{
    if (workspace == nil || owner == nil) {
        return nil;
    }
    native := load Fircompiler Fircompiler->PATH;
    if (native == nil) {
        return nil;
    }
    requests := chan of Request;
    spawn serve(workspace, native, owner, requests);
    return requests;
}
