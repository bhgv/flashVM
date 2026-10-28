# Exclusive Dis browser state; filesystem ownership stays behind Idefiles channels.
Idebrowser: module
{
    PATH: con "idebrowser.dis"; # Reusable bytecode module, with no native UI implementation.
    Panel: con 9; # Read-only selected-entry presentation, interpreted by the renderer.
    PathPanel: con 10; # Editable directory-path field, governed by existing Idefield policy.
    PickDirectory: con 1; # Choose navigated into the selected subdirectory.
    PickFile: con 2; # Choose returned a file path; the caller still owns dirty/load policy.
    MaxRequest: con 2147483647; # Reserve the final request identity for terminal Shutdown.


    # Owned exclusively by one Dis controller; never shared with native rendering.
    State: adt
    {
        endpoint: chan of Idefiles->DirectoryRequest; # Private file-service request endpoint.
        path: string; # Complete native path; preview truncation never changes this value.
        entries: array of Sys->Dir; # Current transferred batch, at most Idefiles.MaxBatch.
        cursor: big; # Positive endpoint-local cursor or zero after clear/error/shutdown.
        selected: int; # Zero-based index within entries, zero for an empty batch.
        page: int; # One-based native batch number, zero before a successful open.
        ended: int; # Last advance reached EOF; current entries remain selectable.
        status: int; # Last Idefiles status; no document revision is stored here.
        message: string; # Copied failure-only diagnostic, rendered as plain escaped text.
        sequence: int; # Next nonnegative request identity; never wraps.
    };


    # Borrow initialized files/sys modules. Return an exclusive owner state or nil.
    create: fn(files: Idefiles, sys: Sys): ref State;


    # Open a candidate path and first batch before retiring the old cursor.
    # Failure preserves the old path/selection; native I/O may block the VM owner.
    open: fn(files: Idefiles, state: ref State, path: string): int;


    # Advance one native batch; End preserves the last visible entries.
    # Native failure retires the cursor and clears its now-unusable entry selection.
    advance: fn(files: Idefiles, state: ref State): int;


    # Move by -1/+1 within the current batch. No I/O; bounds clamp, never wrap.
    move: fn(state: ref State, delta: int): int;


    # Return PickDirectory after navigation, PickFile with a complete native path,
    # or zero on failure. This function never reads/writes a source document.
    choose: fn(files: Idefiles, state: ref State): (int, string);


    # Return a control-escaped UTF-8 preview below the renderer's 1024-byte bound.
    # sys/state are borrowed; full path and entry values remain unchanged.
    summary: fn(sys: Sys, state: ref State): string;


    # Close one cursor before returning; keep the service available for another path.
    clear: fn(files: Idefiles, state: ref State): int;


    # Close every native resource and acknowledge process termination exactly once.
    # Repeated calls are safe; no request is sent to the retired endpoint.
    shutdown: fn(files: Idefiles, state: ref State): int;
};
