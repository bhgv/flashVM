# Copyright (c) 2026 bhgv
# EN: Caller-owned bounded call-chain display, assembled from one immutable ticket.
# RU: Caller-owned bounded call-chain display, собранный из одного immutable ticket.
Residentstack: module
{
    PATH: con "residentstack.dis"; # Resolve the library beside the application image.


    # EN/RU: One top/return position, with copied UTF-8 labels and no native addresses.
    # Одна top/return position, с copied UTF-8 подписями без native addresses.
    Frame: adt {
        pc: big; # Next PC for top; saved return PC for callers.
        instructions: int; # Original Dis instruction extent.
        flags: int; # Processctl LocationValid/truncation bits.
        name: string; # Caller-owned display label, not module authority.
        path: string; # Caller-owned complete UTF-8 prefix.
    };


    # EN/RU: All entries share one owner boundary; flags classify complete/truncated/unavailable.
    # Все entries одной owner boundary; flags различают complete/truncated/unavailable.
    Stack: adt {
        sequence: big; # Saturating owner-local boundary sequence.
        flags: int; # Processctl StackComplete/Truncated/Unavailable, zero before first boundary.
        frames: array of ref Frame; # Bounded copied prefix, top first.
    };


    # EN: Submit/copy/forget; attempts 1..1000 bounds polls, not total latency.
    # Status failure/timeout returns nil; managed errors attempt forget and rethrow.
    # RU: Submit/copy/forget; attempts 1..1000 ограничивает polls, не общую latency.
    # Status failure/timeout возвращает nil; managed errors пытаются forget и rethrow.
    inspect: fn(id: big, attempts: int): (int, ref Stack);
};
