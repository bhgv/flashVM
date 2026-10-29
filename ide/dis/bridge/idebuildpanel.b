implement Idebuildpanel;
include "idecore.m";
include "ideworkspace.m";
include "fircompiler.m";
include "idecompiler.m";
include "idebuildpanel.m";


# Send one editor operation and consume its private reply. Inputs are borrowed;
# no ownership changes and only this process blocks. Return the actual snapshot.
doccall(document: chan of Idecore->Request, operation, revision, first, last: int): Idecore->Reply
{
    answers := chan of Idecore->Reply;
    document <-= Idecore->Request(1, operation, revision, first, last, "", "", answers);
    return <-answers;
}


# Complete one workspace rendezvous; the private response endpoint never escapes.
# owner/request are borrowed, arrays stay immutable until acknowledgement.
workcall(owner: chan of Ideworkspace->Request, request: Ideworkspace->Request): Ideworkspace->Reply
{
    request.reply = chan of Ideworkspace->Reply;
    owner <-= request;
    return <-request.reply;
}


# Return an independent mutable array. source is borrowed and may be nil; no
# native heap pointers escape. Allocation failure follows ordinary Dis semantics.
copybytes(source: array of byte): array of byte
{
    result := array[len source] of byte;
    result[:] = source;
    return result;
}


# Own build results and the private workspace until terminal acknowledgement.
# All modules/endpoints are borrowed live roots. Each build snapshots the editor,
# compiles a private copy, then rechecks the editor revision before publication.
# [GC:] Saved arrays belong exclusively to this process; replies receive copies.
serve(core: Idecore, document: chan of Idecore->Request, workspace: Ideworkspace,
      owner: chan of Ideworkspace->Request, compiler: Idecompiler,
      worker: chan of Idecompiler->Request, requests: chan of Request)
{
    built := 0;
    builtlanguage := 0;
    builttarget := 0;
    bytes: array of byte;
    debug: array of byte;
    attempted := 0;
    laststage := -1;
    diagnostic := "";
    offset := -1;
    length := 0;
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        snapshot := doccall(document, core->Snapshot, 0, 0, 0);
        response := Reply(request.id, Invalid, -1, snapshot.revision, 0, 0,
                          "", snapshot, nil, nil);
        if (request.id < 0) {
            request.reply <-= response;
            continue;
        }
        if (request.operation == Shutdown) {
            answers := chan of Idecompiler->Reply;
            worker <-= Idecompiler->Request(1, compiler->Shutdown, 0, 0, 0, 0, 0, answers);
            stopped := <-answers;
            empty := Idecore->Request(0, core->Snapshot, 0, 0, 0, "", "", nil);
            job := Ideworkspace->Request(1, workspace->Shutdown, 0, "", "", empty,
                                        0, 0, "", 0, nil, 1, nil);
            retired := workcall(owner, job);
            if (stopped.status != workspace->Ok || retired.status != workspace->Ok) {
                raise "fail: compiler panel private owner shutdown";
            }
            response.status = Ok;
            request.reply <-= response;
            return;
        }
        if (request.operation < Build || request.operation > Diagnostic) {
            request.reply <-= response;
            continue;
        }
        if (!snapshot.opened || snapshot.status != core->Ok || snapshot.revision != request.revision) {
            response.status = Conflict;
            request.reply <-= response;
            continue;
        }
        if (request.operation == Artifact) {
            response.status = NoArtifact;
            if (built == snapshot.revision) {
                response.status = Ok;
                response.compiler = Fircompiler->Ok;
                response.language = builtlanguage;
                response.target = builttarget;
                response.artifact = copybytes(bytes);
                response.debug = copybytes(debug);
            }
        } else if (request.operation == Diagnostic) {
            response.status = NoDiagnostic;
            if (attempted == snapshot.revision && offset >= 0 && length >= 0 &&
                offset <= 2147483647 - length) {
                response.document = doccall(document, core->Utf8Span, snapshot.revision,
                                            offset, offset + length);
                response.compiler = laststage;
                response.diagnostic = diagnostic;
                if (response.document.status == core->Ok) {
                    response.status = Ok;
                } else if (response.document.status == core->Conflict) {
                    response.status = Conflict;
                }
            }
        } else if (request.language >= Fircompiler->AS1 && request.language <= Fircompiler->Limbo &&
                   request.target >= Fircompiler->Dis && request.target <= Fircompiler->Swf &&
                   (request.optimize == 0 || request.optimize == 1)) {
            empty := Idecore->Request(0, core->Snapshot, 0, 0, 0, "", "", nil);
            job := Ideworkspace->Request(1, workspace->Open, 0, "/editor-build", snapshot.text,
                                        empty, 0, 0, "", 0, nil, 0, nil);
            opened := workcall(owner, job);
            if (opened.status != workspace->Ok) {
                raise "fail: compiler panel private snapshot";
            }
            answers := chan of Idecompiler->Reply;
            worker <-= Idecompiler->Request(request.id, compiler->Build, opened.document,
                                            opened.core.revision, request.language,
                                            request.target, request.optimize, answers);
            result := <-answers;
            response.compiler = result.compiler;
            response.language = request.language;
            response.target = request.target;
            response.diagnostic = result.diagnostic;
            current := doccall(document, core->Snapshot, 0, 0, 0);
            response.document = current;
            response.status = Conflict;
            if (current.opened && current.revision == snapshot.revision && current.status == core->Ok) {
                attempted = snapshot.revision;
                laststage = result.compiler;
                diagnostic = result.diagnostic;
                offset = -1;
                length = 0;
                response.status = BuildFailed;
                if (result.status == workspace->Ok && result.compiler == Fircompiler->Ok) {
                    job.operation = workspace->Artifact;
                    job.document = opened.document;
                    artifact := workcall(owner, job);
                    if (artifact.status != workspace->Ok) {
                        raise "fail: compiler panel admitted artifact";
                    }
                    bytes = artifact.artifact;
                    debug = result.debug;
                    built = snapshot.revision;
                    builtlanguage = request.language;
                    builttarget = request.target;
                    response.status = Ok;
                } else if (result.status == workspace->BuildFailed && result.kind != 0) {
                    offset = result.offset;
                    length = result.length;
                }
            }
            # This source owner is private and disposable even for failed/stale jobs.
            job.operation = workspace->Close;
            job.document = opened.document;
            job.revision = opened.core.revision;
            job.force = 1;
            closed := workcall(owner, job);
            if (closed.status != workspace->Ok) {
                raise "fail: compiler panel snapshot retirement";
            }
        }
        request.reply <-= response;
    }
}


# Establish private services before exposing an endpoint. Return nil on missing
# inputs/native compiler; retire the newly created workspace on that path.
# The borrowed editor remains live after panel shutdown or unavailable startup.
start(core: Idecore, document: chan of Idecore->Request,
      workspace: Ideworkspace, compiler: Idecompiler): chan of Request
{
    if (core == nil || document == nil || workspace == nil || compiler == nil) {
        return nil;
    }
    owner := workspace->start(core);
    worker := compiler->start(workspace, owner);
    if (worker == nil) {
        empty := Idecore->Request(0, core->Snapshot, 0, 0, 0, "", "", nil);
        job := Ideworkspace->Request(1, workspace->Shutdown, 0, "", "", empty,
                                    0, 0, "", 0, nil, 1, nil);
        stopped := workcall(owner, job);
        if (stopped.status != workspace->Ok) {
            raise "fail: unavailable compiler panel workspace shutdown";
        }
        return nil;
    }
    requests := chan of Request;
    spawn serve(core, document, workspace, owner, compiler, worker, requests);
    return requests;
}
