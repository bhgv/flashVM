# Workspace ownership stays on Dis: documents, history and compile-result revisions.
Ideworkspace: module
{
    PATH: con "ideworkspace.dis";        # Module resource selected by the host namespace.
    MaxDocuments: con 16;                # Maximum simultaneous document/history owners.
    MaxArtifactBytes: con 2097152;       # Maximum binary result admitted for one document.
    MaxTotalArtifactBytes: con 8388608;  # Aggregate retained compilation-result budget.
    MaxDiagnosticCharacters: con 65536;  # Maximum plain-text diagnostic response.

    Open,          # Create a new document/history owner.
    Command,       # Forward an allowed idecore request through its private endpoint.
    List,          # Return copied metadata for every document.
    BeginBuild,    # Capture source, revision, target and a fresh build ticket.
    FinishBuild,   # Admit a worker result only for its still-current source revision.
    Artifact,      # Copy a successful binary only while its source revision remains current.
    Close,         # Stop one private owner after revision/dirty checks.
    Shutdown: con iota;  # Stop all private owners and acknowledge terminal shutdown.

    Ok,            # Workspace operation completed; inspect core.status for a forwarded command.
    Invalid,       # Invalid request, path or result representation.
    NotFound,      # Unknown document or absent successful result.
    Conflict,      # Duplicate path, stale revision or superseded build ticket.
    Limit,         # Document, ticket, diagnostic or artifact budget exceeded.
    Dirty,         # Close/shutdown needs explicit permission to discard unsaved edits.
    BuildFailed: con iota;  # Worker reported compilation failure without replacing a successful result.


    # Copied metadata; no mutable workspace entry escapes to a caller.
    Entry: adt
    {
        document: int;  # Stable positive workspace-lifetime document ID.
        path: string;   # Canonical logical source path, not an OS filesystem capability.
        revision: int;  # Current idecore content revision.
        saved: int;     # Revision acknowledged as saved by idecore.
    };


    # Every admitted caller must consume its single reply before abandoning the request.
    # Input arrays and referenced values must remain unchanged until acknowledgement.
    Request: adt
    {
        id: int;                    # Nonnegative correlation ID.
        operation: int;             # Workspace operation above.
        document: int;              # Stable document ID for operations after Open.
        path: string;               # Open: canonical absolute source identity.
        text: string;               # Open: initial source; FinishBuild: plain diagnostics.
        command: Idecore->Request;  # Command: forwarded value; its reply is privately replaced.
        revision: int;              # BeginBuild/FinishBuild/Close: exact source revision.
        ticket: int;                # FinishBuild: ticket returned by BeginBuild.
        target: string;             # BeginBuild: "dis", "swf" or "fir".
        success: int;               # FinishBuild: one for success, zero for diagnosed failure.
        artifact: array of byte;    # FinishBuild: binary output, copied before acknowledgement.
        force: int;                 # Close/Shutdown: one explicitly permits discarding dirty text.
        reply: chan of Reply;       # Required rendezvous destination; nil requests are ignored.
    };


    # All arrays and entries are independent copies; strings are immutable Dis values.
    Reply: adt
    {
        id: int;                      # Echoed correlation ID.
        status: int;                  # Workspace status, separate from core.status.
        document: int;                # Selected/created document ID or zero.
        core: Idecore->Reply;         # Complete document result; revision is zero when unavailable.
        entries: array of ref Entry;  # List: copied document metadata; nil otherwise.
        ticket: int;                  # Build ticket identifying the exact source snapshot.
        target: string;               # Compiler result format.
        text: string;                 # BeginBuild source snapshot, or retained build diagnostics.
        artifact: array of byte;      # Artifact: independent copy of the successful binary result.
    };


    # core is a loaded idecore module. Returns a new ordinary Dis request channel,
    # or nil for a nil module. Acknowledged Shutdown is terminal; no cancellation
    # of a caller blocked on its reply channel is provided by this interface.
    start: fn(core: Idecore): chan of Request;
};
