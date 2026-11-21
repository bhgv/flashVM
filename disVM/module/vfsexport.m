# Copyright (c) 2026 bhgv
# EN: Local opaque VFS trees and publication capabilities; no wire pointers or caller-supplied identity.
# RU: Local opaque VFS trees и publication capabilities; без wire pointers и подставной identity.
Vfsexport: module
{
    PATH: con "$Vfsexport"; # Matched hosted builtin, separate from Inferno Sys ABI.
    Prepare: con 1; # Defer each request for explicit owner review.
    Tree: con 1; # overlay delegates descendants;0 publishes only the exact node.
    Read: con 4; # Read access ceiling.
    Write: con 2; # Write access ceiling.
    Execute: con 1; # Execute/search access ceiling.
    Pending: con 0; # Request awaits owner decision.
    Granted: con 1; # Tree is ready for one-use take.
    Denied: con 2; # Owner refused or closed.
    Consumed: con -1; # Grant consumed or ticket cancelled.

    # EN/RU: Public prefix has no authority; only native-created exact types are accepted.
    # Public prefix не даёт authority; принимаются только exact native types.
    Root: adt { reserved: int; }; # Immutable provider capability, shareable through typed channels.
    Owner: adt { reserved: int; }; # Publication review/reply/close authority.
    Endpoint: adt { reserved: int; }; # Request authority only; cannot review or grant.
    Ticket: adt { reserved: int; }; # One requester result/cancellation capability.
    Request: adt { reserved: int; }; # Immutable owner-side copied request and publisher identity.

    # EN/RU: Open canonical relative name directly on root; modes OREAD/OWRITE/ORDWR, no creation.
    # Открыть relative name без namespace lookup; owned FD/nil, без create/truncate/fallback.
    open: fn(root: ref Root, path: string, mode: int): ref Sys->FD;

    # EN/RU: Create an exact native int32 file capability.
    # Создать точную файловую ссылку на ячейку int32.
    intvar: fn(initial: int): ref Root;
    intarray: fn(cells: list of ref Root): ref Root;
    record: fn(fields: list of (string, ref Root)): ref Root;
    # Source uses the opaque reference layout of Bigtable.Handle.
    table: fn(source: ref Root): ref Root;
    intget: fn(root: ref Root): (int,int);
    intset: fn(root: ref Root, value: int): int;

    # EN/RU: Current process's inherited native launch channel grant; owned root/nil, no mount/lookup.
    # Grant своего process из native handoff; owned root/nil, без mount/namespace lookup.
    terminal: fn(): ref Root;

    # EN/RU: Copied immutable receive0/send1 name from that same launch; nil if absent/invalid direction.
    # Copied имя receive0/send1 своего launch; nil при отсутствии/неверном direction.
    terminalpath: fn(direction: int): string;

    # EN/RU: Current launch's read-only code/data service, scoped to this process namespace; root/nil.
    # Read-only code/data service своего launch и текущего namespace process; root/nil.
    terminalmovie: fn(): ref Root;


    # EN/RU: Capture one effective signed layer from own namespace; rights1..7; nil on refusal.
    # Захватить один effective layer по signed index; не весь набор descendant bind.
    capture: fn(path: string, index, rights: int): ref Root;

    # EN/RU: New immutable tree: base plus path mapped to source; nil source hides prefix.
    # Новое immutable tree: base плюс mapping; nil base starts empty, rights/flags explicit.
    overlay: fn(base, source: ref Root, path: string, rights, flags: int): ref Root;

    # EN/RU: Current root metadata before mount:0 success/-1 failure, rwx mode and directory1/file0.
    # Metadata capability до mount; не permission grant и не snapshot будущего I/O.
    info: fn(root: ref Root): (int,int,int);

    # EN/RU: Combined mount/bind in caller namespace;0 success,-2 stale,-1 refusal.
    # File targets may be absent leaves under existing directories; siblings remain visible.
    # Mount/bind capability своего namespace; для файла указать полное имя, siblings сохраняются.
    mount: fn(root: ref Root, target: string, flags, index: int): int;

    # EN/RU: Create bounded publication; Prepare requires owner reply, direct requires root.
    # Создать publication; limit1..256; direct endpoint explicitly delegates prechosen root.
    announce: fn(limit, flags: int, direct: ref Root): ref Owner;

    # EN/RU: Derive request-only endpoint; sharing it explicitly delegates request admission.
    # Получить request-only endpoint; не передаёт Owner authority.
    endpoint: fn(owner: ref Owner): ref Endpoint;

    # EN/RU: Ordinary readonly notification node; bind it to receive automatic VFS sysevents.
    # Readonly notification node; bind подключает automatic VFS sysevents без guest polling.
    events: fn(owner: ref Owner): ref Root;

    # EN/RU: Submit copied logical name; native layer captures actual permanent caller PID.
    # Отправить copied name/kind/rights; PID берётся native, peer/remote flags от guest нет.
    submit: fn(endpoint: ref Endpoint, name: string, kind, rights: int): ref Ticket;

    # EN/RU: Take exactly one owner request, nil when none; native request is immutable.
    # Получить ровно один запрос, nil если пусто; GC unanswered request denies it.
    next: fn(owner: ref Owner): ref Request;

    # EN/RU: Copied review id, actual local requester PID, kind and access ceiling.
    # Copied id/PID/kind/rights; integer id alone never redeems or grants anything.
    describe: fn(request: ref Request): (big,int,int,int);

    # EN/RU: One UTF-8 logical-name byte;1/byte,0/end,-1 invalid offset/request.
    # Один UTF-8 byte имени; никаких host source paths.
    text: fn(request: ref Request, offset: int): (int,int);

    # EN/RU: Exact matching owner/request, chosen root or nil deny;1 accepted,0 gone/closed.
    # Owner/request должны относиться к одной публикации; source может отличаться от запроса.
    reply: fn(owner: ref Owner, request: ref Request, chosen: ref Root): int;

    # EN/RU: Nonwaiting ticket state; granted tree stays owned until take or cancel/GC.
    # Читать state без ожидания; повторный status не поглощает grant.
    status: fn(ticket: ref Ticket): int;

    # EN/RU: Transfer granted tree once; nil pending/denied/consumed. Check status first.
    # Получить granted tree один раз; nil для pending/denied/consumed.
    take: fn(ticket: ref Ticket): ref Root;

    # EN/RU: Cancel ticket once;1 changed,0 already cancelled; aliases share the same state.
    # Отменить ticket; все aliases разделяют terminal state.
    cancel: fn(ticket: ref Ticket): int;

    # EN/RU: Close publication; accepted trees survive, pending denied, endpoints stop admission.
    # Закрыть publication; issued trees живут, pending denied, endpoint больше не принимает.
    close: fn(owner: ref Owner): int;
};
