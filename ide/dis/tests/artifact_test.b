implement ArtifactTest;
include "sys.m";
include "idecore.m";
include "ideworkspace.m";
include "fircompiler.m";
include "idecompiler.m";
include "idebuildpanel.m";
include "idefileops.m";
include "ideartifact.m";
ArtifactTest: module
{
    init: fn(); # Compile, export, reread and reject stale/mislabeled/existing outputs.
};


# Enforce an explicit contract. Values are borrowed; failure raises a named error.
check(condition: int, message: string)
{
    if (!condition) {
        raise "fail: artifact " + message;
    }
}


# Consume exactly one panel reply; request/endpoint are borrowed live values.
buildcall(endpoint: chan of Idebuildpanel->Request, request: Idebuildpanel->Request): Idebuildpanel->Reply
{
    request.reply = chan of Idebuildpanel->Reply;
    endpoint <-= request;
    return <-request.reply;
}


# Consume exactly one export reply, including terminal Shutdown acknowledgement.
exportcall(endpoint: chan of Ideartifact->Request, request: Ideartifact->Request): Ideartifact->Reply
{
    request.reply = chan of Ideartifact->Reply;
    endpoint <-= request;
    return <-request.reply;
}


# Compare binary values without string conversion; nil/length/byte order matter.
same(left, right: array of byte): int
{
    if (len left != len right) {
        return 0;
    }
    for (index := 0; index < len left; index++) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}


# Run on the real VM in a caller-selected private writable directory. The native
# test driver supplies existing file and symlink sentinels; no fixture owns the
# user's workspace. All service lifetimes end before reporting successful export.
init()
{
    sys := load Sys Sys->PATH;
    core := load Idecore Idecore->PATH;
    workspace := load Ideworkspace Ideworkspace->PATH;
    compiler := load Idecompiler Idecompiler->PATH;
    panel := load Idebuildpanel Idebuildpanel->PATH;
    files := load Idefileops Idefileops->PATH;
    artifacts := load Ideartifact Ideartifact->PATH;
    check(sys != nil && core != nil && workspace != nil && compiler != nil && panel != nil &&
          files != nil && artifacts != nil, "modules");
    raw := array[256] of byte;
    for (index := 0; index < len raw; index++) {
        raw[index] = byte index;
    }
    check(files->writebytes(sys, "raw.bin", raw, 1) == files->Ok, "binary create");
    (status, read) := files->readbytes(sys, "raw.bin", 256);
    check(status == files->Ok && same(raw, read), "all byte values");
    (status, read) = files->readbytes(sys, "raw.bin", 255);
    check(status == files->TooLarge && read == nil, "bounded binary read");
    (textstatus, text) := files->read(sys, "raw.bin", 256);
    check(textstatus == files->Invalid && text == "", "strict source reader unchanged");
    check(files->writebytes(sys, "invalid.bin", raw, 2) == files->Invalid, "invalid exclusive flag");
    check(files->writebytes(sys, "bad\0.bin", raw, 1) == files->Invalid, "embedded path NUL");
    document := core->start();
    documents := chan of Idecore->Reply;
    source := "# π😀 export\nimplement M;M:module{value:fn():int;};value():int{return 6*7;}";
    document <-= Idecore->Request(1, core->Open, 0, 0, 0, source, "", documents);
    opened := <-documents;
    builds := panel->start(core, document, workspace, compiler);
    check(builds != nil, "panel start");
    exports := artifacts->start(sys, files, panel, builds);
    check(exports != nil, "export start");
    job := Ideartifact->Request(21, artifacts->Export, opened.revision, Fircompiler->Dis,
                                artifacts->Binary, "absent.dis", nil);
    result := exportcall(exports, job);
    check(result.status == artifacts->NoArtifact && result.size == 0, "no build no file");
    endings := array[] of {"", ".dis", ".fir", ".swf"};
    for (target := Fircompiler->Dis; target <= Fircompiler->Swf; target++) {
        build := buildcall(builds, Idebuildpanel->Request(2, panel->Build, opened.revision,
                           Fircompiler->Limbo, target, 1, nil));
        check(build.status == panel->Ok, "build");
        snapshot := buildcall(builds, Idebuildpanel->Request(3, panel->Artifact, opened.revision, 0, 0, 0, nil));
        job.target = target;
        job.part = artifacts->Binary;
        job.path = "export" + string target + endings[target];
        result = exportcall(exports, job);
        check(result.id == 21 && result.status == artifacts->Ok && result.target == target &&
              result.size == len snapshot.artifact, "binary export metadata");
        (status, read) = files->readbytes(sys, job.path, Idefileops->MaxBytes);
        check(status == files->Ok && same(read, snapshot.artifact), "binary roundtrip");
        result = exportcall(exports, job);
        check(result.status == artifacts->Io && result.size == 0, "repeat never overwrites");
        job.part = artifacts->Debug;
        job.path = "export" + string target + ".firdbg";
        result = exportcall(exports, job);
        check(result.status == artifacts->Ok && result.size == len snapshot.debug, "debug export");
        (status, read) = files->readbytes(sys, job.path, Idefileops->MaxBytes);
        check(status == files->Ok && same(read, snapshot.debug), "debug roundtrip");
    }
    job.part = artifacts->Binary;
    job.path = "sentinel.swf";
    result = exportcall(exports, job);
    check(result.status == artifacts->Io, "existing destination");
    job.path = "linked.swf";
    result = exportcall(exports, job);
    check(result.status == artifacts->Io, "symlink destination");
    job.path = "dangling.swf";
    result = exportcall(exports, job);
    check(result.status == artifacts->Io, "dangling symlink destination");
    job.path = "wrong.dis";
    result = exportcall(exports, job);
    check(result.status == artifacts->Invalid, "format suffix");
    job.target = Fircompiler->Dis;
    result = exportcall(exports, job);
    check(result.status == artifacts->WrongTarget && result.target == Fircompiler->Swf, "actual retained format");
    job.target = Fircompiler->Swf;
    job.path = "missing/out.swf";
    result = exportcall(exports, job);
    check(result.status == artifacts->Io, "missing parent");
    document <-= Idecore->Request(4, core->Replace, opened.revision, 0, 0, "# changed\n", "", documents);
    edited := <-documents;
    job.path = "stale.swf";
    result = exportcall(exports, job);
    check(result.status == artifacts->Conflict, "old revision rejected");
    job.revision = edited.revision;
    result = exportcall(exports, job);
    check(result.status == artifacts->NoArtifact, "new revision has no artifact");
    job.operation = artifacts->Shutdown;
    result = exportcall(exports, job);
    check(result.status == artifacts->Ok, "export shutdown");
    stopped := buildcall(builds, Idebuildpanel->Request(5, panel->Shutdown, 0, 0, 0, 0, nil));
    check(stopped.status == panel->Ok, "panel shutdown");
    document <-= Idecore->Request(6, core->Shutdown, edited.revision, 0, 0, "", "", documents);
    closed := <-documents;
    check(closed.status == core->Ok && closed.saved == opened.saved, "document unchanged saved state");
    sys->print("PASS artifact export: targets=3 binary=ok debug=ok exclusive=ok stale=reject shutdown=ok\n");
}
