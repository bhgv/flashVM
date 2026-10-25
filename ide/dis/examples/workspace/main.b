implement WorkspaceExample; # Implement the callable educational module.
include "idecore.m";        # Use the real document/history protocol.
include "ideworkspace.m";   # Use the real workspace/build-ticket protocol.
include "main.m";           # Keep the exported signature in its module header.


# Open a document, obtain a revision-bound build snapshot, and stop every owner.
# core/workspace borrow already loaded modules. Returns one on success, zero on failure.
# This demonstrates coordination; an actual compiler must consume the returned job.
run(core: Idecore, workspace: Ideworkspace): int
{ # Enter the portable example without loading an OS or UI module.
    owner := workspace->start(core); # Spawn a workspace and its future document owners on Dis.
    if (owner == nil) { # Reject a missing document implementation.
        return 0; # No service was created, so no teardown is needed.
    } # Continue only with a live owner channel.
    response := chan of Ideworkspace->Reply; # Retain this rendezvous until each reply arrives.
    command := Idecore->Request(1, Idecore->Snapshot, 0, 0, 0, "", "", nil); # Initialize every scalar explicitly.
    request := Ideworkspace->Request(1, Ideworkspace->Open, 0, "/src/example.b", "source text", command, # Begin a fully initialized request.
                                    0, 0, "", 0, nil, 0, response); # Set the complete value, including unused fields.
    owner <-= request; # Ask the workspace to allocate a private document/history owner.
    opened := <-response; # Finish the exchange before changing or abandoning the request.
    ok := opened.status == Ideworkspace->Ok; # Record failure but still perform terminal cleanup.
    if (ok) { # A build snapshot needs a successfully opened document.
        request.operation = Ideworkspace->BeginBuild; # Request coordination, not pretend compilation.
        request.document = opened.document; # Select the stable document ID returned by Open.
        request.revision = opened.core.revision; # Bind the job to the exact current source revision.
        request.target = "dis"; # Declare the intended artifact format for the real compiler worker.
        owner <-= request; # Transfer the request through a normal blocking Dis channel.
        job := <-response; # Receive the immutable source, target and unique build ticket.
        ok = job.status == Ideworkspace->Ok && job.ticket > 0 && job.text == "source text"; # Check the snapshot.
    } # No binary output is fabricated by this coordination example.
    request.operation = Ideworkspace->Shutdown; # Stop workspace and every private document owner.
    request.force = 1; # Explicitly allow teardown even if a future extension edits this document.
    owner <-= request; # Send exactly one terminal request.
    stopped := <-response; # Consume the acknowledgement before dropping the channel.
    return ok && stopped.status == Ideworkspace->Ok; # Report both functional and lifecycle success.
} # Finish without native handles crossing either VM heap.
