# Revision-bound compiler panel for an existing editor document, entirely on Dis.
Idebuildpanel: module
{
    PATH: con "idebuildpanel.dis"; # Portable module; native compiler remains optional.
    Build, # Compile the exact requested editor revision through the private workspace.
    Artifact, # Copy the last successful result only while its editor revision is current.
    Diagnostic, # Convert the last failed source span through the actual document owner.
    Shutdown: con iota; # Stop private compiler/workspace owners and acknowledge terminal shutdown.
    Ok, # Operation completed; inspect compiler for native stage on Build.
    Invalid, # Invalid request/options; existing results stay intact.
    Conflict, # Editor revision changed or differs from the requested revision.
    NoArtifact, # No successful binary belongs to this current revision.
    NoDiagnostic, # No current source diagnostic has a selectable byte span.
    BuildFailed: con iota; # Compiler reported an admitted failure.


    # Requests borrow immutable values until their unique acknowledgement.
    Request: adt
    {
        id: int; # Nonnegative correlation identity.
        operation: int; # One operation above.
        revision: int; # Exact editor revision; ignored for Shutdown.
        language: int; # Build: Fircompiler AS1..Limbo.
        target: int; # Build: Fircompiler Dis/Fir/Swf.
        optimize: int; # Build: zero or one.
        reply: chan of Reply; # Required rendezvous; callers must consume the reply.
    };


    # Copied metadata; binary/debug arrays are returned only by Artifact.
    Reply: adt
    {
        id: int; # Original correlation identity.
        status: int; # Panel status above, separate from compiler stage.
        compiler: int; # Native stage or -1 when no compilation was attempted.
        revision: int; # Editor revision owning this result.
        language: int; # Language of selected build/result.
        target: int; # Actual binary format, never inferred from a filename.
        diagnostic: string; # Plain native text; never interpreted as markup.
        document: Idecore->Reply; # Snapshot or validated diagnostic character selection.
        artifact: array of byte; # Artifact: independent copied binary, nil otherwise.
        debug: array of byte; # Artifact: independent original FIRDBG1, nil otherwise.
    };


    # Borrow core/document and loaded workspace/compiler implementations. Return a
    # new channel or nil if any input/native compiler is unavailable. The service
    # owns its private workspace/compiler, never the editor document. Every call
    # blocks; the native parser blocks the sole VM owner. No cancellation exists.
    start: fn(core: Idecore, document: chan of Idecore->Request,
              workspace: Ideworkspace, compiler: Idecompiler): chan of Request;
};
