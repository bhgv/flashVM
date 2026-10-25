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
};
