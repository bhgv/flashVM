# Hosted NC64 guest process control.
Processctl: module
{
    PATH: con "$Processctl";
    NotWaiting: con 0;
    Cancelled: con 1;
    Denied: con -1;
    Requested: con 1;
    Retired: con 1;

    # Return the caller's runtime process ID.
    pid: fn(): int;

    # Cancel a current wait of a direct child. Missing and foreign IDs are denied.
    cancelwait: fn(pid: int): int;

    # Request termination of a direct child at a managed boundary.
    kill: fn(pid: int): int;

    # Request termination and wait for that child's managed resources to retire.
    killwait: fn(pid: int): int;
};
