# Copyright (c) 2026 bhgv
# EN: Caller-namespace effective layer inspection and explicit ordered bind/move.
# RU: Просмотр effective layers своего namespace и explicit ordered bind/move.
Vfsctl: module
{
    PATH: con "vfsctl.dis"; # Pure Limbo convenience layer over $Vfsnative.
    MREPL: con 0; # Replace target stack.
    MBEFORE: con 1; # Insert at the top.
    MAFTER: con 2; # Insert at the bottom.
    MCREATE: con 4; # Mark branch as a classic create candidate.
    MAUTORWX: con 8; # Explicit effective-access selection policy.
    MINS: con 16; # Vfsctl-only indexed insertion, separate index argument.
    Ready: con 1; # Inspection card/list exists; mutation success is0.
    Error: con -1; # Refused/invalid operation.
    Stale: con -2; # Namespace revision changed; inspect again before mutation.
    Native: con 0; # Retained native anchor route.
    Provider: con 1; # Retained provider factory route.
    Execute: con 1; # Effective execute/search access bit.
    Write: con 2; # Effective write access bit.
    Read: con 4; # Effective read access bit; known query bits live at <<8.
    Stablebind: con 1; # Observed route is stable or already captured dot.
    Capturebind: con 2; # Provider supports explicit per-node capture; actual capture may refuse.


    # EN/RU: Copied configuration and live observations, no native pointers/host source paths.
    # Configuration fenced by revision; metadata/access/source state can change after query.
    # Copied config и live observations; не полномочия и не atomic provider snapshot.
    Layer: adt
    {
        id, head: big; # Group-local binding/head identities; id0 is implicit native route.
        position, count: int; # Zero-based top-to-bottom position and effective candidate count.
        flags, kind: int; # Bind/create/access policy and Native/Provider routing kind.
        metadata, mode, directory, access: int; # Port metadata status, rwx/mode, directory, allowed|(known<<8).
        mount, relative: string; # Virtual mount point and capability-relative route, never host absolute source.
        session, generation: big; # Local provider journal state, zero for native routes.
        capabilities: int; # Native provider promises; not permission authority.
    };


    # EN/RU: Load native service once before sharing this module; raise on missing service.
    # Загрузить native service до sharing module; raise при отказе.
    init: fn();


    # EN/RU: Inspect one signed position; revision=-1 captures current, -2 means stale.
    # Return status,revision,Layer; missing position gives0/nil. No mutation.
    # Читать signed position; revision=-1 — current; вернуть status/revision/Layer.
    layer: fn(path: string, revision: big, index: int): (int,big,ref Layer);


    # EN/RU: Return all effective providers in traversal order at one namespace revision.
    # Bounded8 stale retries, no partial list on failure. Per-provider attributes are live.
    # Вернуть всех providers по порядку;8 retries, failure без partial list.
    layers: fn(path: string): (int,big,list of ref Layer);


    # EN/RU: Bind source stack: standard top/bottom/replace plus MINS with signed slot.
    # Index0 otherwise. 0 success,-2 stale,-1 failure; use Sys.bind for standard ABI.
    # Exact file targets may be absent under existing directories; parent/siblings remain visible.
    # Bind source stack: MINS signed slot, иначе index0; file target — полное имя, return0/-2/-1.
    bind: fn(source, target: string, flags, index: int): int;


    # EN/RU: Move observed binding to final signed position, guarded by revision.
    # Inherited file/subdirectory stack becomes an independent exact-path binding.
    # Старый parent/siblings не меняются;0 success,-2 stale,-1 failure.
    move: fn(path: string, revision, id: big, index: int): int;
};
