# Build one real workspace snapshot without spawning a host compiler process.
implement CompilerExample; # Match the standalone entry declared below.
include "sys.m"; # Use native printing only for the final tutorial acknowledgement.
include "idecore.m"; # Describe the document owner and its revisioned commands.
include "ideworkspace.m"; # Describe snapshot tickets and artifact admission.
include "fircompiler.m"; # Select the explicit language/format library contract.
include "idecompiler.m"; # Use the portable Limbo worker's request channel.

CompilerExample: module { # Declare the actual Dis application entry.
    init: fn(); # No host CLI argument or graphics context is required.
}; # End the entry interface.


# Stop borrowed worker/workspace owners after a successful or failed tutorial.
# Either endpoint may be nil. Return one after every admitted owner acknowledges
# terminal shutdown; no caller sends on those channels again.
stop(owner: chan of Ideworkspace->Request, worker: chan of Idecompiler->Request): int
{ # Teardown remains ordinary Dis channel traffic, including local operations.
    ok := 1; # Preserve an earlier shutdown failure while trying the other owner.
    if (worker != nil) { # A failed optional start creates no worker to close.
        built := chan of Idecompiler->Reply; # Keep one terminal reply endpoint live.
        worker <-= Idecompiler->Request(3, Idecompiler->Shutdown, 0, 0, 0, 0, 0, built); # Stop after prior jobs.
        result := <-built; # Consume the unique worker acknowledgement.
        ok = result.status == Ideworkspace->Ok; # Record actual terminal admission.
    } # The terminated worker endpoint is never reused.
    if (owner != nil) { # Workspace lifetime is independent of compiler availability.
        reply := chan of Ideworkspace->Reply; # Wait for private document shutdowns too.
        command := Idecore->Request(1, Idecore->Snapshot, 0, 0, 0, "", "", nil); # Supply the neutral copied command field.
        owner <-= Ideworkspace->Request(4, Ideworkspace->Shutdown, 0, "", "", command, 0, 0, "", 0, nil, 1, reply); # Explicitly discard this transient tutorial document.
        result := <-reply; # Consume the workspace's terminal acknowledgement.
        ok = ok && result.status == Ideworkspace->Ok; # Preserve either owner's failure.
    } # All admitted teardown exchanges are complete.
    return ok; # Report genuine cleanup instead of silently abandoning an owner.
} # Both endpoints are borrowed; no native pointers or threads are retained.


# Compile a bounded numeric source through real workspace/worker channels.
# No parameters. Modules/endpoints remain rooted on this stack until every owner
# acknowledges shutdown; failures raise instead of advertising a fabricated build.
init()
{ # All policy below executes as Dis bytecode on our interpreter/JIT.
    sys := load Sys Sys->PATH; # Resolve the native diagnostic output module.
    core := load Idecore Idecore->PATH; # Load the real document/history service.
    workspace := load Ideworkspace Ideworkspace->PATH; # Load the ticket/revision owner.
    compiler := load Idecompiler Idecompiler->PATH; # Load the Limbo build worker.
    if (sys == nil || core == nil || workspace == nil || compiler == nil) { # Require every actual dependency.
        raise "fail: compiler tutorial modules unavailable"; # Do not fall back to subprocesses or empty stubs.
    } # Continue only with checked module signatures.
    owner := workspace->start(core); # Spawn a private workspace without UI authority.
    worker := compiler->start(workspace, owner); # Load the optional builtin and spawn its sequential worker.
    if (worker == nil) { # OFF packages deliberately report the missing capability.
        stop(owner, worker); # Close the workspace even when optional start failed.
        raise "fail: enable the Fircompiler native library service"; # Make this an actionable failure.
    } # The loaded native module stays rooted by the worker's server stack.
    command := Idecore->Request(1, Idecore->Snapshot, 0, 0, 0, "", "", nil); # Supply a copied neutral document command.
    source := "implement M;M:module{value:fn():int;};value():int{return 6*7;}"; # Keep the bounded source result independently known.
    reply := chan of Ideworkspace->Reply; # Own the single workspace reply rendezvous.
    request := Ideworkspace->Request(1, Ideworkspace->Open, 0, "/src/example.b", source, command, 0, 0, "", 0, nil, 0, reply); # Open one canonical logical identity.
    owner <-= request; # Send copied document data to its real owner.
    opened := <-reply; # Consume the unique acknowledgement before proceeding.
    if (opened.status != Ideworkspace->Ok) { # Require a real revisioned document.
        stop(owner, worker); # Balance both owners before reporting the original failure.
        raise "fail: compiler tutorial open"; # Stop instead of inventing an initial revision.
    } # The returned revision is authoritative.
    built := chan of Idecompiler->Reply; # Own the worker's terminal reply endpoint.
    job := Idecompiler->Request(2, Idecompiler->Build, opened.document, opened.core.revision, Fircompiler->Limbo, Fircompiler->Dis, 1, built); # Request optimized Dis for this exact snapshot.
    worker <-= job; # The worker performs BeginBuild, library compilation and FinishBuild.
    result := <-built; # This synchronous reply does not promise responsive UI during compilation.
    if (result.status != Ideworkspace->Ok || result.compiler != Fircompiler->Ok || len result.debug == 0) { # Check both policy and native stages.
        stop(owner, worker); # Retire the service before exposing the failed build.
        raise "fail: compiler tutorial build"; # Reject native or stale-result failures explicitly.
    } # Original FIRDBG1 is available without claiming a final native-PC map.
    request.operation = Ideworkspace->Artifact; # Ask the owner for its admitted result.
    request.document = opened.document; # Address the created document rather than the old Open request.
    owner <-= request; # Fetch through the workspace channel, not native memory.
    artifact := <-reply; # Receive another independent byte-array copy.
    if (artifact.status != Ideworkspace->Ok || artifact.target != "dis" || len artifact.artifact == 0) { # Refuse absent or relabeled bytes.
        stop(owner, worker); # Reject absent bytes with balanced owner teardown.
        raise "fail: compiler tutorial artifact"; # Do not automatically load or run an unadmitted result.
    } # Saving/execution is a separate future client decision.
    if (!stop(owner, worker)) { # Stop worker first, then its still-separate workspace.
        raise "fail: compiler tutorial shutdown"; # Require both terminal acknowledgements.
    } # Every successful tutorial request/reply has completed.
    sys->print("PASS compiler tutorial: revision=ok artifact=ok shutdown=ok\n"); # Advertise only the verified service behavior.
} # Returning retires the remaining copied results and local roots normally.
