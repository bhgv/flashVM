# Read-only file windows for resource inspection and future debugger/source views.
Idefiles: module
{
    PATH: con "idefiles.dis";  # Module resource resolved beside the application.
    MaxBytes: con 65536;       # Maximum binary window allocated per request.
    MaxPathBytes: con 4096;    # Maximum UTF-8 extent of an ordinary native path.
    Read: con 0;              # Measure and read one binary window without modifying the file.
    Shutdown: con 1;          # Acknowledge and terminate this service owner.
    Ok: con 0;                # Complete result, including a short read or EOF.
    Invalid: con 1;           # Bad protocol values; no native file was opened.
    Failed: con 2;            # Open, seek or read failed; no bytes are published.
    OpenDir: con 2;           # Open one bounded directory cursor; metadata comes from fstat.
    NextDir: con 3;           # Transfer the next native directory batch, at most 32 records.
    CloseDir: con 4;          # Retire one live cursor identity, including an exhausted cursor.
    StatPath: con 5;          # Copy path metadata without consuming a directory slot.
    End: con 3;               # Cursor is exhausted; NextDir remains stable until CloseDir.
    Stale: con 4;             # Cursor identity is absent or was retired; no native operation occurs.
    Full: con 5;              # This owner already holds MaxDirectories live cursor identities.
    MaxDirectories: con 8;    # Per-owner directory quota, independent of the native global FD limit.
    MaxBatch: con 32;         # Sys.dirread port bound; the service never assembles a whole directory.


    # Caller retains immutable input until the single reply is consumed.
    Request: adt
    {
        id: int;               # Nonnegative caller correlation identity.
        operation: int;        # Read or terminal Shutdown.
        path: string;          # Ordinary file path, never a module-store identity.
        offset: big;           # Signed byte offset relative to the selected origin.
        origin: int;           # Sys SEEKSTART, SEEKRELA or SEEKEND; a fresh handle starts at zero.
        count: int;            # Requested byte extent, from zero through MaxBytes.
        reply: chan of Reply;  # Required private response rendezvous; nil requests are ignored.
    };


    # Bytes belong exclusively to the receiver after acknowledgement; nothing is retained.
    Reply: adt
    {
        id: int;               # Echoed correlation identity.
        status: int;           # Ok, Invalid or Failed; independent of document/history state.
        size: big;             # File extent measured before reading, or -1 on failure.
        position: big;         # Actual selected byte offset, or -1 on failure.
        bytes: array of byte;  # Exact read result, including an empty array at EOF.
        message: string;       # Plain diagnostic; never interpreted as markup or code.
    };


    # sys borrows an initialized native Sys module. Returns a new owner channel,
    # or nil for a nil module. Native I/O may block the hosting scheduler owner.
    # Every admitted request requires a drained reply; acknowledged Shutdown is terminal.
    start: fn(sys: Sys): chan of Request;


    # Metadata and directory requests use a distinct endpoint from byte windows.
    # The caller must drain reply; a nil reply is ignored without changing state.
    DirectoryRequest: adt
    {
        id: int;                         # Nonnegative correlation identity, echoed without reinterpretation.
        operation: int;                  # OpenDir, NextDir, CloseDir, StatPath or terminal Shutdown.
        cursor: big;                     # Owner-local identity for NextDir/CloseDir; zero for OpenDir/StatPath.
        path: string;                    # Borrowed immutable native path for OpenDir/StatPath only.
        reply: chan of DirectoryReply;   # Private blocking response endpoint; must be consumed.
    };


    # Records/arrays are independent VM values; no FD or native cursor is exported.
    DirectoryReply: adt
    {
        id: int;                         # Original correlation identity, including invalid requests.
        status: int;                     # Ok, Invalid, Failed, End, Stale or Full.
        cursor: big;                     # Open identity or echoed lookup identity; never a native descriptor.
        metadata: Sys->Dir;              # OpenDir/StatPath metadata; zerodir for other replies.
        entries: array of Sys->Dir;       # Transferred batch, never retained by the service; nil otherwise.
        message: string;                 # Failure-only diagnostic; successful exchange emits no log event.
    };


    # Start one serialized directory owner, or return nil for a nil Sys module.
    # Cursor IDs are scoped to this returned endpoint and never reused by it.
    # Shutdown closes every native FD before acknowledgement. Native I/O may
    # block the VM scheduler owner; this is not asynchronous OS I/O or cancellation.
    startdirs: fn(sys: Sys): chan of DirectoryRequest;
};
