implement Ideartifact;
include "sys.m";
include "idecore.m";
include "ideworkspace.m";
include "fircompiler.m";
include "idecompiler.m";
include "idebuildpanel.m";
include "idefileops.m";
include "ideartifact.m";


# Validate path/suffix before fetching or opening anything. path/ending are
# borrowed immutable strings. No canonicalization or filesystem sandbox is implied.
validpath(path, ending: string): int
{
    if (len path <= len ending || len array of byte path > 4096 ||
        path[len path - len ending:] != ending) {
        return 0;
    }
    for (index := 0; index < len path; index++) {
        if (path[index] == 0) {
            return 0;
        }
    }
    return 1;
}


# Serialize explicit exports; all modules/endpoints stay rooted on this server
# stack. A single immutable artifact fetch precedes an OS-exclusive create.
# [GC:] Fetched arrays are independent Dis copies, never cross-heap pointers.
serve(sys: Sys, files: Idefileops, panel: Idebuildpanel,
      builds: chan of Idebuildpanel->Request, requests: chan of Request)
{
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        response := Reply(request.id, Invalid, request.revision, 0, 0);
        if (request.id < 0) {
            request.reply <-= response;
            continue;
        }
        if (request.operation == Shutdown) {
            response.status = Ok;
            request.reply <-= response;
            return;
        }
        if (request.operation != Export || request.target < Fircompiler->Dis ||
            request.target > Fircompiler->Swf || (request.part != Binary && request.part != Debug)) {
            request.reply <-= response;
            continue;
        }
        ending := ".dis";
        if (request.target == Fircompiler->Fir) {
            ending = ".fir";
        } else if (request.target == Fircompiler->Swf) {
            ending = ".swf";
        }
        if (request.part == Debug) {
            ending = ".firdbg";
        }
        if (!validpath(request.path, ending)) {
            request.reply <-= response;
            continue;
        }
        answers := chan of Idebuildpanel->Reply;
        builds <-= Idebuildpanel->Request(request.id, panel->Artifact, request.revision, 0, 0, 0, answers);
        artifact := <-answers;
        response.status = NoArtifact;
        if (artifact.status == panel->Conflict) {
            response.status = Conflict;
        } else if (artifact.status == panel->Ok) {
            response.target = artifact.target;
            response.status = WrongTarget;
            if (artifact.target == request.target) {
                bytes := artifact.artifact;
                if (request.part == Debug) {
                    bytes = artifact.debug;
                }
                response.status = Invalid;
                if (len bytes > 0 && len bytes <= Idefileops->MaxBytes) {
                    response.status = Io;
                    if (files->writebytes(sys, request.path, bytes, 1) == Idefileops->Ok) {
                        response.status = Ok;
                        response.size = len bytes;
                    }
                }
            }
        }
        request.reply <-= response;
    }
}


# Spawn one owner without creating files or acquiring ownership of borrowed
# services. A nil dependency returns nil; allocation/spawn use ordinary VM errors.
start(sys: Sys, files: Idefileops, panel: Idebuildpanel,
      builds: chan of Idebuildpanel->Request): chan of Request
{
    if (sys == nil || files == nil || panel == nil || builds == nil) {
        return nil;
    }
    requests := chan of Request;
    spawn serve(sys, files, panel, builds, requests);
    return requests;
}
