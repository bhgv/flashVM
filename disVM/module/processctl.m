# Copyright (c) 2026 bhgv
# EN: Hosted NC64 process control and read-only live resident metadata.
# RU: Hosted NC64 process control и read-only live resident metadata.
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

    # EN: Read one copied resident after a nonnegative lifetime cursor in this VM.
    # Return status (1 card, 0 end, -1 unavailable/invalid), id, PID, done.
    # Pages are observational; IDs grant no kill/debug/memory authority.
    # RU: Читать copied resident после nonnegative lifetime cursor в этой VM.
    # Вернуть status (1 card, 0 end, -1 unavailable/invalid), id, PID, done.
    # Pages observational; IDs не дают kill/debug/memory authority.
    resident: fn(after: big): (int, big, int, int);

    Starting: con 0;
    Dispatch: con 1;
    Scheduler: con 2;
    WaitWork: con 3;
    Teardown: con 4;
    Done: con 5;

    # EN: Same cursor/status/zeroed refusal as resident; append owner phase.
    # Phase is an interval, not an OS state, wait cause or heap access permission.
    # RU: Те же cursor/status/нулевой отказ, что resident; добавить owner phase.
    # Phase — интервал, не OS state, причина ожидания либо heap permission.
    residentstate: fn(after: big): (int, big, int, int, int);

    UnknownActivity: con 0; # Unrecognized publication.
    ReadyActivity: con 1; # Ready hint, not OS CPU state.
    SendActivity: con 2; # Channel send registration.
    ReceiveActivity: con 3; # Channel receive registration.
    AltActivity: con 4; # Channel alt registration.
    NativeActivity: con 5; # Released native continuation.
    TableActivity: con 6; # Cooperative table admission.
    ChildActivity: con 7; # Child retirement/cancellation reply.
    DebugActivity: con 8; # Retained debug state only.
    ExitingActivity: con 9; # Published exit.
    BrokenActivity: con 10; # Published failure.

    # EN: Query a positive exact lifetime ID; return status (1 found, 0 gone,
    # -1 invalid/unavailable) and copied activity (zero unless found).
    # Independent from phase; not a channel identity, pin or OS CPU state.
    # RU: Запросить positive exact lifetime ID; вернуть status (1 найден, 0 исчез,
    # -1 invalid/unavailable) и copied activity (ноль кроме found).
    # Независимо от phase; не channel identity, pin либо OS CPU state.
    residentactivity: fn(id: big): (int, int);

};
