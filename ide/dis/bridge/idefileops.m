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

    # Read arbitrary binary bytes with the same bounded path/FD ownership as read.
    # Return (status, independent bytes); failures return nil, never partial output.
    readbytes: fn(sys: Sys, path: string, limit: int): (int, array of byte);

    # Write immutable binary bytes to an explicit path. exclusive must be 0/1;
    # one uses OEXCL, preserving existing files/symlinks without a stat/open race.
    # Validation precedes create. I/O failure may leave a partial newly created
    # file; success is not fsync/durability. Native I/O blocks the sole owner.
    writebytes: fn(sys: Sys, path: string, bytes: array of byte, exclusive: int): int;

    # Write a complete UTF-8 snapshot to an explicit path, handling short writes.
    # Returns status and closes its local FD. Success is checked overwrite, not atomic replace.
    # Invalid path/oversize validation happens before create; I/O failure may leave a partial file.
    write: fn(sys: Sys, path: string, text: string): int;
};
