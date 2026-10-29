# Build-job policy stays on Dis channels; native code supplies libraries only.
Idecompiler: module
{
    PATH: con "idecompiler.dis"; # Portable channel worker, built with matching interfaces.
    Build, # Begin/snapshot, compile and finish one revision-bound workspace job.
    Shutdown: con iota; # Acknowledge after all prior builds; endpoint is then terminal.


    # Borrowed request values must remain immutable until the unique reply.
    Request: adt
    {
        id: int; # Nonnegative caller correlation identity.
        operation: int; # Build or Shutdown.
        document: int; # Workspace document identity.
        revision: int; # Exact expected source revision.
        language: int; # Fircompiler AS1/AS2/AS3/Limbo constant.
        target: int; # Fircompiler Dis/Fir/Swf constant.
        optimize: int; # Exactly zero or one.
        reply: chan of Reply; # Required rendezvous; consume before abandoning call.
    };


    # Copied result metadata; the artifact is fetched separately through workspace.
    Reply: adt
    {
        id: int; # Original request correlation identity.
        status: int; # Workspace status; Ok does not mean native compiler success.
        compiler: int; # Native stage, or -1 if no compilation was attempted.
        ticket: int; # Snapshot ticket, zero if BeginBuild failed.
        diagnostic: string; # Plain copied native diagnostic; never markup.
        debug: array of byte; # Fresh original FIRDBG1 only after admitted success.
        kind: int; # Public source diagnostic kind, zero for other failures.
        line: int; # One-based diagnostic line, zero if absent.
        offset: int; # Zero-based original UTF-8 byte offset, -1 if absent.
        length: int; # Diagnostic byte length, zero if absent.
    };


    # Load the optional builtin and spawn one sequential worker. workspace/owner
    # are borrowed live module/endpoint roots. Return nil if unavailable or nil
    # inputs. Compilation blocks the sole VM owner; no cancellation/parallel parser
    # calls. Worker owns no document or UI and never shuts down the workspace.
    start: fn(workspace: Ideworkspace, owner: chan of Ideworkspace->Request): chan of Request;
};
