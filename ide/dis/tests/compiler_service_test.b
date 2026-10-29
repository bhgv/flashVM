# Executable service tutorial and acceptance: real libraries, jobs and byte output.
implement CompilerService;
include "sys.m";
include "idecore.m";
include "ideworkspace.m";
include "fircompiler.m";
include "idecompiler.m";

CompilerService: module
{
    init: fn(); # Exercise native builds and the actual Limbo workspace worker.
};

sys: Sys; # Native diagnostics and explicit tutorial output-file capability.


# Raise on a failed contract; condition/message are borrowed scalar/string values.
# No successful placeholder or external emulator is used; return only on success.
check(condition: int, message: string)
{
    if (!condition) {
        raise "fail: compiler service " + message;
    }
}


# Save a complete tutorial artifact to the caller-selected working directory.
# path/bytes are borrowed and stay immutable; close the temporary descriptor by
# returning. Raise on any short write. Worker policy itself never writes files.
save(path: string, bytes: array of byte)
{
    fd := sys->create(path, Sys->OWRITE, 8r666);
    check(fd != nil && sys->write(fd, bytes, len bytes) == len bytes, "write artifact");
}


# Send one workspace value and consume its unique reply; endpoint is borrowed.
# Return copied metadata/bytes. No private document endpoint is obtained.
workspacecall(owner: chan of Ideworkspace->Request, request: Ideworkspace->Request): Ideworkspace->Reply
{
    request.reply = chan of Ideworkspace->Reply;
    owner <-= request;
    return <-request.reply;
}


# Send one compiler-worker value and consume the single terminal reply.
# channel is borrowed; acknowledged Shutdown makes it terminal and unreusable.
workercall(channel: chan of Idecompiler->Request, request: Idecompiler->Request): Idecompiler->Reply
{
    request.reply = chan of Idecompiler->Reply;
    channel <-= request;
    return <-request.reply;
}


# Run native int/AS builds, diagnostics, copy lifetime and revision-bound jobs.
# No parameters. Native compilation blocks the owner; all application policy and
# workspace interactions remain actual Limbo rendezvous operations. Print PASS
# only after both worker and document owner acknowledge terminal shutdown.
init()
{
    sys = load Sys Sys->PATH;
    native := load Fircompiler Fircompiler->PATH;
    core := load Idecore Idecore->PATH;
    workspace := load Ideworkspace Ideworkspace->PATH;
    worker := load Idecompiler Idecompiler->PATH;
    check(sys != nil && native != nil && core != nil && workspace != nil && worker != nil, "load modules");
    limbo := "# π😀 build snapshot\nimplement M;M:module{value:fn():int;};value():int{sum:=0;for(i:=0;i<7;i++){sum+=6;}return sum;}";
    action := "// π😀 build snapshot\nvar x=6;trace(x*7);";
    retained: array of byte;
    retainedtext := "";
    for (language := 1; language <= 4; language++) {
        source := action;
        if (language == Fircompiler->Limbo) {
            source = limbo;
        }
        input := array of byte source;
        for (optimize := 0; optimize <= 1; optimize++) {
            for (target := 1; target <= 3; target++) {
                (status, artifact, debug, message, kind, line, offset, length) :=
                    native->compile(language, target, optimize, input);
                check(status == Fircompiler->Ok && len artifact > 0 && len debug > 0 &&
                      message == "" && kind == 0 && line == 0 && offset == -1 && length == 0, "native success");
                check(string input == source, "input immutable");
                if (retained != nil) {
                    check(string retained == retainedtext, "previous result lifetime");
                }
                retained = debug;
                retainedtext = string debug;
                name := sys->sprint("compiler-%d-%d-%d", language, optimize, target);
                save(name, artifact);
                save(name + ".debug", debug);
            }
        }
    }
    bad := array of byte "# π😀 failure\nimplement M;M:module{value:fn():int;};value():int{return missing;}";
    (stage, bytes, debug, message, kind, line, offset, length) :=
        native->compile(Fircompiler->Limbo, Fircompiler->Dis, 0, bad);
    check(stage == Fircompiler->Source && bytes == nil && debug == nil && kind != 0 &&
          line == 2 && offset > 0 && length == 7 && message != "", "located source rejection");
    save("compiler-error-source", bad);
    save("compiler-error-location", array of byte sys->sprint("%d %d %d\n", line, offset, length));
    (stage, bytes, debug, message, kind, line, offset, length) = native->compile(1 << 40, 1, 0, bad);
    check(stage == Fircompiler->Invalid && bytes == nil && debug == nil, "wide invalid option");
    (stage, bytes, debug, message, kind, line, offset, length) =
        native->compile(4, 1, 0, array[Fircompiler->MaxSource + 1] of byte);
    check(stage == Fircompiler->Limit && bytes == nil && debug == nil, "source budget");
    owner := workspace->start(core);
    channel := worker->start(workspace, owner);
    check(owner != nil && channel != nil, "worker start");
    command := Idecore->Request(1, Idecore->Snapshot, 0, 0, 0, "", "", nil);
    request := Ideworkspace->Request(1, Ideworkspace->Open, 0, "/src/π.b", limbo,
                                     command, 0, 0, "", 0, nil, 0, nil);
    opened := workspacecall(owner, request);
    check(opened.status == Ideworkspace->Ok, "workspace open");
    build := Idecompiler->Request(7, Idecompiler->Build, opened.document,
                                  opened.core.revision, 4, 1, 1, nil);
    built := workercall(channel, build);
    check(built.id == 7 && built.status == Ideworkspace->Ok && built.compiler == Fircompiler->Ok &&
          built.ticket > 0 && len built.debug > 0, "worker build");
    request.operation = Ideworkspace->Artifact;
    request.document = opened.document;
    artifact := workspacecall(owner, request);
    check(artifact.status == Ideworkspace->Ok && artifact.target == "dis" && len artifact.artifact > 0, "admitted artifact");
    first := artifact.artifact[0];
    artifact.artifact[0] = byte 0;
    copied := workspacecall(owner, request);
    check(copied.artifact[0] == first, "workspace artifact copy");
    build.revision++;
    rejected := workercall(channel, build);
    check(rejected.status == Ideworkspace->Conflict && rejected.compiler == -1 && rejected.debug == nil,
          "stale job not compiled");
    build.revision--;
    build.target = Fircompiler->Fir;
    rebuilt := workercall(channel, build);
    check(rebuilt.status == Ideworkspace->Ok && rebuilt.ticket > built.ticket, "new target ticket");
    request.operation = Ideworkspace->Artifact;
    copied = workspacecall(owner, request);
    check(copied.target == "fir" && string copied.artifact[:3] == "FIR", "target preserved");
    build.language = Fircompiler->AS1;
    failed := workercall(channel, build);
    check(failed.status == Ideworkspace->BuildFailed && failed.compiler == Fircompiler->Source &&
          failed.debug == nil && failed.diagnostic != "", "diagnosed worker failure");
    copied = workspacecall(owner, request);
    check(copied.status == Ideworkspace->Ok && copied.target == "fir", "failed job retains last artifact");
    request.operation = Ideworkspace->BeginBuild;
    request.revision = opened.core.revision;
    request.target = "dis";
    pending := workspacecall(owner, request);
    check(pending.status == Ideworkspace->Ok, "manual snapshot");
    request.operation = Ideworkspace->Command;
    request.command.operation = Idecore->Replace;
    request.command.expected = opened.core.revision;
    request.command.first = 0;
    request.command.last = 0;
    request.command.text = "# edited\n";
    edited := workspacecall(owner, request);
    check(edited.status == Ideworkspace->Ok && edited.core.status == Idecore->Ok, "edit during pending job");
    (stage, bytes, debug, message, kind, line, offset, length) =
        native->compile(4, 1, 0, array of byte pending.text);
    check(stage == Fircompiler->Ok, "compile old snapshot honestly");
    request.operation = Ideworkspace->FinishBuild;
    request.ticket = pending.ticket;
    request.revision = pending.core.revision;
    request.success = 1;
    request.artifact = bytes;
    check(workspacecall(owner, request).status == Ideworkspace->Conflict, "late real artifact rejected");
    request.operation = Ideworkspace->Artifact;
    check(workspacecall(owner, request).status == Ideworkspace->Conflict, "edited source hides previous artifact");
    build.operation = Idecompiler->Shutdown;
    check(workercall(channel, build).status == Ideworkspace->Ok, "worker terminal acknowledgement");
    request.operation = Ideworkspace->Shutdown;
    request.force = 1;
    check(workspacecall(owner, request).status == Ideworkspace->Ok, "workspace terminal acknowledgement");
    sys->print("PASS compiler service: native=24 workspace=ok spans=ok copies=ok shutdown=ok\n");
}
