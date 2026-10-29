# Explicit immutable build-result export; all application policy executes on Dis.
Ideartifact: module
{
    PATH: con "ideartifact.dis"; # Portable server using borrowed typed compiler/file interfaces.
    Export, # Fetch a current artifact and exclusively create one explicit output file.
    Shutdown: con iota; # Acknowledge terminal retirement; borrowed services remain live.
    Binary, # Export the compiled container, requiring .dis/.fir/.swf as appropriate.
    Debug: con iota; # Export original FIRDBG1, requiring .firdbg; not a target PC map.
    Ok, # All bytes accepted by the file service; no durability promise.
    Invalid, # Invalid correlation, operation, part, path or suffix; no file is opened.
    Conflict, # Requested editor revision is stale.
    NoArtifact, # No successful build exists for this revision.
    WrongTarget, # Retained artifact format differs from the explicit requested format.
    Io: con iota; # Exclusive creation/write failed; existing files are never overwritten.


    # Borrow input values unchanged until the unique reply. No cancellation exists.
    Request: adt
    {
        id: int; # Nonnegative caller correlation identity.
        operation: int; # Export or Shutdown.
        revision: int; # Exact editor revision checked by the build panel.
        target: int; # Expected Fircompiler Dis/Fir/Swf; never inferred from arbitrary bytes.
        part: int; # Binary or Debug; files are separate explicit transactions.
        path: string; # Explicit native UTF-8 destination, never an implicit source path.
        reply: chan of Reply; # Required acknowledgement endpoint.
    };


    # Copied metadata; no mutable compiler arrays or native descriptors escape.
    Reply: adt
    {
        id: int; # Original request identity.
        status: int; # Export status above.
        revision: int; # Requested source revision, including diagnosed failures.
        target: int; # Actual retained target if fetched, otherwise zero.
        size: int; # Complete accepted byte count on Ok; zero otherwise, not partial-file length.
    };


    # Borrow Sys, binary file library and live panel endpoint. Return nil for nil
    # inputs, otherwise a new FIFO server. It owns no editor/workspace/panel and
    # must stop before the borrowed panel. Native I/O blocks the VM owner. Export
    # is bound to the artifact snapshot admitted at fetch; later edits do not
    # cancel an already admitted write. Failed writes may leave partial NEW files.
    start: fn(sys: Sys, files: Idefileops, panel: Idebuildpanel,
              builds: chan of Idebuildpanel->Request): chan of Request;
};
