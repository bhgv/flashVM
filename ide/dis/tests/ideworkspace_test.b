implement IdeworkspaceTest;
include "sys.m";
include "idecore.m";
include "ideworkspace.m";
include "main.m";

sys: Sys;                 # Reference diagnostics/control; not a production workspace dependency.
core: Idecore;            # Actual document/history service implementation.
workspace: Ideworkspace;  # Actual workspace owner under test.


# Direct reference-runtime entry requires no Draw or shell initialization.
IdeworkspaceTest: module
{
    init: fn(); # Load services, test real rendezvous channels and halt the isolated reference VM.
};


# Halt the explicitly provided isolated test runtime; returns never.
halt()
{
    control := sys->open("/dev/sysctl", Sys->OWRITE);
    if (control == nil) {
        raise "fail: workspace halt unavailable";
    }
    bytes := array of byte "halt";
    sys->write(control, bytes, len bytes);
    raise "fail: workspace halt returned";
}


# Check a concrete owner result; message describes the failed contract. Returns on success only.
check(condition: int, message: string)
{
    if (!condition) {
        sys->print("FAIL ideworkspace %s\n", message);
        halt();
    }
}


# Send one value to the owner and synchronously receive its unique acknowledgement.
# channel is borrowed; request is copied and its reply replaced. Returns the response.
call(channel: chan of Ideworkspace->Request, request: Ideworkspace->Request): Ideworkspace->Reply
{
    request.id = 1;
    request.reply = chan of Ideworkspace->Reply;
    channel <-= request;
    return <-request.reply;
}


# Exercise document isolation, private metadata, stale builds, artifacts and dirty shutdown.
# No parameters. Prints PASS after all owner processes acknowledge cleanup.
init()
{
    sys = load Sys Sys->PATH;
    core = load Idecore Idecore->PATH;
    workspace = load Ideworkspace Ideworkspace->PATH;
    check(core != nil && workspace != nil, "load modules");
    example := load WorkspaceExample WorkspaceExample->PATH;
    check(example != nil && example->run(core, workspace), "callable tutorial ownership and snapshot");
    owner := workspace->start(core);
    command := Idecore->Request(1, Idecore->Snapshot, 0, 0, 0, "", "", nil);
    request := Ideworkspace->Request(1, Ideworkspace->Open, 0, "", "", command, 0, 0, "", 0, nil, 0, nil);
    request.operation = Ideworkspace->Open;
    request.path = "/src/Я.b";
    request.text = "alpha";
    opened := call(owner, request);
    check(opened.status == Ideworkspace->Ok && opened.core.text == "alpha", "first open");
    first := opened.document;
    request.path = "/src/other.as";
    request.text = "beta";
    second := call(owner, request);
    check(second.status == Ideworkspace->Ok && second.document != first, "independent document ID");
    request.path = "/src/Я.b";
    check(call(owner, request).status == Ideworkspace->Conflict, "duplicate source identity");
    request.path = "/src/../bad";
    check(call(owner, request).status == Ideworkspace->Invalid, "path traversal rejected");
    request.operation = Ideworkspace->List;
    listing := call(owner, request);
    check(listing.status == Ideworkspace->Ok && len listing.entries == 2, "document list");
    listing.entries[0].path = "mutated";
    check(call(owner, request).entries[0].path != "mutated", "metadata is independently copied");

    request.document = first;
    request.operation = Ideworkspace->BeginBuild;
    request.revision = opened.core.revision;
    request.target = "dis";
    job := call(owner, request);
    check(job.status == Ideworkspace->Ok && job.text == "alpha" && job.ticket > 0, "exact build snapshot");
    request.operation = Ideworkspace->Command;
    request.command.operation = Idecore->Replace;
    request.command.expected = opened.core.revision;
    request.command.first = 0;
    request.command.last = 5;
    request.command.text = "changed";
    changed := call(owner, request);
    check(changed.status == Ideworkspace->Ok && changed.core.status == Idecore->Ok, "document edit");
    check(job.text == "alpha", "worker source snapshot remains immutable");
    request.operation = Ideworkspace->FinishBuild;
    request.ticket = job.ticket;
    request.success = 1;
    request.artifact = array of byte "old";
    check(call(owner, request).status == Ideworkspace->Conflict, "stale compilation rejected");

    request.operation = Ideworkspace->BeginBuild;
    request.revision = changed.core.revision;
    job = call(owner, request);
    newer := call(owner, request);
    check(newer.ticket > job.ticket, "newer build supersedes an older ticket");
    request.operation = Ideworkspace->FinishBuild;
    request.ticket = job.ticket;
    check(call(owner, request).status == Ideworkspace->Conflict, "superseded ticket rejected");
    request.ticket = newer.ticket;
    request.artifact = array[Ideworkspace->MaxArtifactBytes + 1] of byte;
    check(call(owner, request).status == Ideworkspace->Limit, "oversized binary rejected without consuming ticket");
    request.artifact = array of byte "binary";
    check(call(owner, request).status == Ideworkspace->Ok, "current artifact admitted");
    request.artifact[0] = byte 'X';
    request.operation = Ideworkspace->Artifact;
    artifact := call(owner, request);
    check(artifact.status == Ideworkspace->Ok && string artifact.artifact == "binary" &&
          artifact.target == "dis" && artifact.ticket == newer.ticket, "artifact copies input bytes and metadata");
    artifact.artifact[0] = byte 'Y';
    check(string call(owner, request).artifact == "binary", "artifact replies cannot mutate retained bytes");

    request.operation = Ideworkspace->BeginBuild;
    request.target = "fir";
    pending := call(owner, request);
    request.operation = Ideworkspace->Artifact;
    check(call(owner, request).target == "dis", "pending target cannot relabel successful bytes");
    request.operation = Ideworkspace->FinishBuild;
    request.ticket = pending.ticket;
    request.success = 0;
    request.text = "compiler diagnostic";
    check(call(owner, request).status == Ideworkspace->BuildFailed, "compile failure is explicit");
    request.operation = Ideworkspace->Artifact;
    artifact = call(owner, request);
    check(artifact.status == Ideworkspace->Ok && artifact.target == "dis", "failed build retains earlier valid artifact");

    request.operation = Ideworkspace->Command;
    request.command.operation = Idecore->Replace;
    request.command.expected = changed.core.revision;
    request.command.first = 0;
    request.command.last = 7;
    request.command.text = "third";
    edited := call(owner, request);
    check(edited.core.status == Idecore->Ok, "edit after successful build");
    request.revision = edited.core.revision;
    request.operation = Ideworkspace->Artifact;
    check(call(owner, request).status == Ideworkspace->Conflict, "edited source cannot expose a stale artifact");
    request.operation = Ideworkspace->Open;
    request.text = "";
    for (i := 2; i < Ideworkspace->MaxDocuments; i++) {
        request.path = "/src/extra" + string i + ".b";
        check(call(owner, request).status == Ideworkspace->Ok, "bounded additional document");
    }
    request.path = "/src/overflow.b";
    check(call(owner, request).status == Ideworkspace->Limit, "document quota");
    request.operation = Ideworkspace->Command;
    request.command.operation = Idecore->Shutdown;
    check(call(owner, request).status == Ideworkspace->Invalid, "private owner cannot be shut down through forwarding");
    request.command.operation = Idecore->Snapshot;
    request.document = second.document;
    check(call(owner, request).core.text == "beta", "other document unaffected");
    request.operation = Ideworkspace->Shutdown;
    request.force = 0;
    check(call(owner, request).status == Ideworkspace->Dirty, "dirty shutdown is atomic and rejected");
    request.operation = Ideworkspace->List;
    check(len call(owner, request).entries == Ideworkspace->MaxDocuments, "rejected shutdown closes no documents");
    request.document = first;
    request.operation = Ideworkspace->Close;
    check(call(owner, request).status == Ideworkspace->Dirty, "dirty close requires explicit force");
    request.force = 1;
    check(call(owner, request).status == Ideworkspace->Ok, "forced close acknowledges private owner teardown");
    request.operation = Ideworkspace->Shutdown;
    check(call(owner, request).status == Ideworkspace->Ok, "terminal workspace acknowledgement");
    sys->print("PASS ideworkspace documents, tickets, copies, stale builds and shutdown\n");
    halt();
}
