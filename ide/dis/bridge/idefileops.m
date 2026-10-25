# Reusable checked source-file operations; filesystem policy remains with the Dis caller.
Idefileops: module
{
    PATH: con "idefileops.dis"; # Matching source-built Dis library, independent of any UI.
    Ok, Invalid, TooLarge, Io: con iota; # Explicit status; no failed read is an empty successful source.
    MaxBytes: con 4194304;      # Bounded library byte input/output, not a filesystem size claim.

    # Read strict UTF-8 up to limit bytes; sys/path are borrowed, limit is 1..MaxBytes.
    # Returns (status, owned text); failed reads return empty text and close their local FD.
    # Native I/O may block the sole owner; no document or output file is modified.
    read: fn(sys: Sys, path: string, limit: int): (int, string);

    # Write a complete UTF-8 snapshot to an explicit path, handling short writes.
    # Returns status and closes its local FD. Success is checked overwrite, not atomic replace.
    # Invalid path/oversize validation happens before create; I/O failure may leave a partial file.
    write: fn(sys: Sys, path: string, text: string): int;
};
