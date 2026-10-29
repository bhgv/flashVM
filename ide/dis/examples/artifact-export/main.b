implement ArtifactExportTutorial; # This client runs as ordinary Dis bytecode.
include "sys.m"; # Native console output only; no compiler subprocess.
include "idecore.m"; # The existing editor/document owner.
include "ideworkspace.m"; # The private compiler snapshot owner implementation.
include "fircompiler.m"; # Explicit language, format and native-stage constants.
include "idecompiler.m"; # The reusable sequential channel worker.
include "idefileops.m"; # Bounded binary I/O with OS-exclusive create.
include "ideartifact.m"; # Explicit artifact export on an ordinary Dis channel.
include "idebuildpanel.m"; # Bind a compiler service to an existing document.

ArtifactExportTutorial: module
{
    init: fn(); # Build one current document and fetch independent result bytes.
};


# Build and explicitly export new example.dis/example.firdbg in the caller cwd.
# No parameters. Each endpoint remains rooted here until its terminal reply.
# Compilation blocks the native VM owner; this example promises no background UI.
init()
{
    sys := load Sys Sys->PATH; # Resolve the actual console capability.
    core := load Idecore Idecore->PATH; # Resolve the portable editor service.
    workspace := load Ideworkspace Ideworkspace->PATH; # Resolve snapshot ownership.
    compiler := load Idecompiler Idecompiler->PATH; # Resolve channel job policy.
    panel := load Idebuildpanel Idebuildpanel->PATH; # Resolve revision admission.
    if (sys == nil || core == nil || workspace == nil || compiler == nil || panel == nil) {
        raise "fail: build panel tutorial modules"; # Never substitute a mocked compiler.
    }
    document := core->start(); # The caller owns this document, not the panel.
    documents := chan of Idecore->Reply; # Receive actual source-state acknowledgements.
    source := "implement M;M:module{value:fn():int;};value():int{return 6*7;}"; # Bounded numeric Limbo.
    document <-= Idecore->Request(1, core->Open, 0, 0, 0, source, "", documents); # Establish saved source.
    opened := <-documents; # Capture the authoritative editor revision.
    endpoint := panel->start(core, document, workspace, compiler); # Create private worker/workspace owners.
    if (endpoint == nil) { # An OFF runtime reports an absent native capability.
        document <-= Idecore->Request(2, core->Shutdown, opened.revision, 0, 0, "", "", documents); # Retire our document.
        retired := <-documents; # Balance the admitted terminal request before failing.
        if (retired.status != core->Ok) {
            raise "fail: build panel tutorial unavailable shutdown";
        }
        raise "fail: enable Fircompiler for the build panel tutorial";
    }
    answers := chan of Idebuildpanel->Reply; # Consume one response per operation.
    request := Idebuildpanel->Request(42, panel->Build, opened.revision, Fircompiler->Limbo,
                                      Fircompiler->Dis, 1, answers); # Explicit optimized Dis output.
    endpoint <-= request; # Snapshot, compile and recheck the same editor revision.
    built := <-answers; # A native success must also pass panel revision admission.
    ok := opened.status == core->Ok && built.status == panel->Ok && built.compiler == Fircompiler->Ok;
    request.operation = panel->Artifact; # Fetch is separate from ordinary metadata replies.
    endpoint <-= request; # No source/result access bypasses the typed channel.
    artifact := <-answers; # Binary/debug arrays are independently owned copies.
    ok = ok && artifact.status == panel->Ok && len artifact.artifact > 0 && len artifact.debug > 0;
    ok = ok && artifact.target == Fircompiler->Dis && artifact.language == Fircompiler->Limbo;
    files := load Idefileops Idefileops->PATH; # Load the binary I/O library.
    exports := load Ideartifact Ideartifact->PATH; # Load portable export policy.
    owner: chan of Ideartifact->Request; # No owner exists until dependencies are checked.
    if (exports != nil && files != nil) { # Resolve capabilities before any call.
        owner = exports->start(sys, files, panel, endpoint); # Borrow the live panel.
    }
    if (owner == nil) { # A missing dependency cannot become a successful export.
        ok = 0;
    } else { # Each output is a separately acknowledged exclusive-create operation.
        replies := chan of Ideartifact->Reply; # Never abandon an admitted request.
        save := Ideartifact->Request(5, exports->Export, opened.revision, Fircompiler->Dis,
                                     exports->Binary, "example.dis", replies); # Explicit NEW binary path.
        owner <-= save; # Fetch only this current revision, then create with OEXCL.
        binary := <-replies; # Existing files remain untouched and report Io.
        ok = ok && binary.status == exports->Ok; # Preserve any previous failure.
        save.part = exports->Debug; # Original FIRDBG1 is a separate requested file.
        save.path = "example.firdbg"; # Never label source debug information .sbl.
        owner <-= save; # This is not an atomic binary/debug pair transaction.
        debug := <-replies; # The result is still not loaded or executed.
        ok = ok && debug.status == exports->Ok;
        save.operation = exports->Shutdown; # Exporter must stop before its borrowed panel.
        owner <-= save; # Complete the final export-owner rendezvous.
        stoppedexport := <-replies; # The endpoint becomes terminal here.
        ok = ok && stoppedexport.status == exports->Ok;
    }
    request.operation = panel->Shutdown; # Retire private compiler owners first.
    endpoint <-= request; # The endpoint becomes terminal after this acknowledgement.
    stopped := <-answers; # Never send another request to the retired endpoint.
    ok = ok && stopped.status == panel->Ok;
    document <-= Idecore->Request(3, core->Shutdown, opened.revision, 0, 0, "", "", documents); # Our editor remains separate.
    retired := <-documents; # Receive its own terminal reply before releasing roots.
    ok = ok && retired.status == core->Ok;
    if (!ok) { # Cleanup has already completed even if compilation failed.
        raise "fail: build panel tutorial result";
    }
    sys->print("PASS artifact tutorial: binary/debug=new revision=ok owners=closed\n"); # Report verified behavior only.
} # Returning releases copied artifacts; only the explicitly requested new files have been published.
