# Copyright (c) 2026 bhgv
# EN: Deadline-bounded managed FD transfers on original cancellable native owners.
# RU: Передачи managed FD с deadline на исходных cancellable native owners.
Proxystream: module
{
    PATH: con "proxystream.dis"; # Matching producer's bytecode IO adapter.
    OK, End, Timeout, Failure: con -iota; # Byte outcomes, never RPC completion.
    Limit: con 4096; # Maximum native transfer staging per request.


    # EN/RU: Initialize immutable Sys/Processctl interfaces before sharing.
    # Инициализировать immutable Sys/Processctl interfaces до sharing.
    init: fn();


    # EN: Transfer exactly len bytes through borrowed FD using one absolute
    # monotonic deadline. Return status and reported progress (-1 on timed-out IO,
    # where syscall bytes may already have committed). Child IO and alarm
    # are joined before return; input bytes stay immutable until both are joined.
    # RU: Передать ровно len bytes через borrowed FD с одним absolute monotonic
    # deadline. Вернуть status/progress (-1 при timeout после возможного commit).
    # IO/alarm children joined до возврата;
    # bytes нельзя менять до join. writing selects write instead of read.
    transfer: fn(fd: ref Sys->FD, bytes: array of byte, writing, deadline: int): (int, int);
};
