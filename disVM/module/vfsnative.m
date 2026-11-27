# Copyright (c) 2026 bhgv
# EN: Scalar native bridge for caller-namespace layer control; prefer Vfsctl wrapper.
# RU: Scalar native bridge управления слоями своего namespace; удобный wrapper Vfsctl.
Scalarmanagedinvoke: module
{
    invoke: fn(wire: array of byte); # Private generated gateway for one compiled export.
};

Scalarimplementation: module
{
    invoke: fn(value: int): int; # Single linked slot; the native loader selects its export.
};

Scalartupleimplementation: module
{
    invoke: fn(value: (int,big)): (int,big); # One linked tuple export.
};

Scalarlistimplementation: module
{
    invoke: fn(seed: int, pair: (int,big), delta: big): (int,(int,big)); # One argument-list export.
};

Scalarrefcell: adt
{
    narrow: int; # Copied signed int.
    wide: big;   # Copied signed big.
};

Scalarrefpairimplementation: module
{
    invoke: fn(first, second: ref Scalarrefcell): (int,big); # One linked two-ref export.
};

Scalarrefimplementation: module
{
    invoke: fn(value: ref Scalarrefcell): (int,big); # One linked ref export.
};

Scalarbigimplementation: module
{
    invoke: fn(value: big): big; # Single linked full-width export.
};

ScalarimplementationSet: module
{
    invoke0: fn(value: int): int; # Slot 0 of one linked module instance.
    invoke1: fn(value: int): int; # Slot 1 of one linked module instance.
    invoke2: fn(value: int): int; # Slot 2 of one linked module instance.
    invoke3: fn(value: int): int; # Slot 3 of one linked module instance.
    invoke4: fn(value: int): int; # Slot 4 of one linked module instance.
    invoke5: fn(value: int): int; # Slot 5 of one linked module instance.
    invoke6: fn(value: int): int; # Slot 6 of one linked module instance.
    invoke7: fn(value: int): int; # Slot 7 of one linked module instance.
};

ScalarbigimplementationSet: module
{
    invoke0: fn(value: big): big;
    invoke1: fn(value: big): big;
    invoke2: fn(value: big): big;
    invoke3: fn(value: big): big;
    invoke4: fn(value: big): big;
    invoke5: fn(value: big): big;
    invoke6: fn(value: big): big;
    invoke7: fn(value: big): big;
};

Scalarloadedanchor: module {}; # Owns the one module data instance for a published set.

Scalarrawinvoke: module
{
    call0: fn(): (big,big,big,big); # Verified zero-cell entry.
    call1: fn(a: big): (big,big,big,big); # Verified one-cell entry.
    call2: fn(a,b: big): (big,big,big,big); # Verified two-cell entry.
    call3: fn(a,b,c: big): (big,big,big,big); # Verified three-cell entry.
    call4: fn(a,b,c,d: big): (big,big,big,big); # Verified four-cell entry.
};

Vfsnative: module
{
    PATH: con "$Vfsnative"; # Hosted builtin, no foreign namespace/PID input.

    # EN/RU: status,revision,head,id,count,position,flags,kind,metadata_status,mode,directory,access.
    # Читать copied card; revision=-1 captures current, stale=-2; no pointer results.
    layer: fn(path: string, revision: big, index: int): (int,big,big,big,int,int,int,int,int,int,int,int);

    # EN/RU: Read mount0/relative1 or launch role2/system3/export4/session5/nickname6, one UTF-8 byte.
    # Читать byte: status1/byte,0/end,-2/stale,-1/error. Never host absolute source paths.
    text: fn(path: string, revision: big, index, part, offset: int): (int,int);

    # EN/RU: status, source session/generation, native capability bits; zero for native anchor.
    # Copied source state, не permission grant и не global registry.
    state: fn(path: string, revision: big, index: int): (int,big,big,int);

    # EN/RU: Bind at a signed insertion slot; flags16=MINS, index0 otherwise.
    # Bind в signed slot; return0 success,-1 error, same source/target validation path.
    bind: fn(source, target: string, flags, index: int): int;

    # EN/RU: Move exact observed ID at revision; inherited child path gets its own stack.
    # Переставить ID по revision; return0 success,-2/stale,-1/error; no effect replay.
    move: fn(path: string, revision, id: big, index: int): int;
    # EN: Run by VFS path. 1 success/delivery, 0 refused, -1 uncertain, -2 owner failure.
    # RU: Запуск по пути VFS. 1 успех/доставка, 0 отказ, -1 неизвестно, -2 ошибка владельца.
    run: fn(path: string): int;
    # EN/RU: Submit a bounded binary startup payload using the selected route.
    # Отправить ограниченные бинарные данные через выбранный маршрут.
    runargs: fn(path: string, arguments: array of byte): int;
    # EN/RU: Send one scalar through the current mounted route and await its reply.
    # Отправить скаляр через текущий смонтированный маршрут и дождаться ответа.
    scalarcall: fn(request, reply: string, requesttag, replytag: big, value: int): int;
    # EN/RU: Read one request from an owner inbox and write one reply through Styx.
    # Прочитать запрос из приёмника владельца и отправить ответ через Styx.
    scalartake: fn(path: string, tag: big): int;
    # EN/RU: Persistent session: (1, argument) or (0, 0) at orderly end or EOF.
    # Постоянная сессия: (1, аргумент) или (0, 0) при завершении либо EOF.
    scalarnext: fn(path: string, tag: big): (int, int);
    scalarreply: fn(path: string, tag: big, success, value: int);
    # EN/RU: Load a local implementation and bind one named scalar export to invoke.
    # Загрузить локальную реализацию и связать именованный экспорт со слотом invoke.
    # EN/RU: Check up to eight tab-separated exports and bind one shared instance.
    # Проверить до восьми экспортов через табуляцию и связать один общий экземпляр.
    scalarloadset: fn(path, names: string): ScalarimplementationSet;
    # EN/RU: Return 1 for a request, 0 at EOF, or -1 if idle.
    # Вернуть 1 для запроса, 0 при EOF или -1 при ожидании.
    scalarpoll: fn(path: string, tag: big): (int, int);
    scalarload: fn(path, exportname: string): Scalarimplementation;
    # EN: Owner-side full-width loading and mailbox I/O.
    # RU: Полноразрядные загрузка и обмен с приёмником владельца.
    scalarbigload: fn(path, exportname: string): Scalarbigimplementation;
    scalarbignext: fn(path: string, tag: big): (int, big);
    scalarbigreply: fn(path: string, tag: big, success: int, value: big);
    # EN: Poll one signed mailbox and link a mixed set into two views of one instance.
    # RU: Проверить знаковый приёмник и связать смешанные типы с одним экземпляром.
    scalarbigpoll: fn(path: string, tag: big): (int, big);
    scalarmixedload: fn(path, names: string): (ScalarimplementationSet, ScalarbigimplementationSet);
    # EN: Original Dis tuple export and its local mailboxes.
    # RU: Исходный экспорт кортежа Dis и его локальные приёмники.
    scalartupleload: fn(path, exportname: string): Scalartupleimplementation;
    scalartuplenext: fn(path: string): (int, int, big);
    scalartuplereply: fn(path: string, success: int, value: (int,big));
    # EN: Original Dis ref export and its local copy-back mailboxes.
    # RU: Исходный экспорт ref Dis и его локальные приёмники обновлений.
    scalarrefload: fn(path, exportname: string): Scalarrefimplementation;
    scalarrefnext: fn(path: string): (int, int, big);
    scalarrefreply: fn(path: string, success: int, value, updated: (int,big));
    # EN: Bind the typed list and exchange its owner-side mailbox records.
    # RU: Связать типизированный список и обменять записи с сервисом владельца.
    scalarlistload: fn(path, exportname: string): Scalarlistimplementation;
    scalarlistnext: fn(path: string): (int,int,int,big,big);
    scalarlistreply: fn(path: string, success, outer, inner: int, wide: big);
    # EN: Bind a two-ref export and exchange its numbered snapshots.
    # RU: Связать экспорт с двумя ref и обменять нумерованные снимки.
    scalarrefpairload: fn(path, exportname: string): Scalarrefpairimplementation;
    scalarrefpairnext: fn(path: string): (int,int,big,int,big);
    scalarrefpairreply: fn(path: string, success, count: int, value, first, second: (int,big));
    # EN: Link one actual export and return exactly one typed view once advertised.
    # RU: Связать фактический экспорт и вернуть одно представление после публикации.
    scalarloaded: fn(path, exportname, offer: string):
        (int, Scalarimplementation, Scalarbigimplementation, Scalartupleimplementation,
              Scalarrefimplementation, Scalarlistimplementation, Scalarrefpairimplementation);
    # EN: Create one data instance and bind single-slot views to it.
    # RU: Создать один экземпляр данных и связать с ним однослотовые представления.
    scalarloadanchor: fn(path: string): Scalarloadedanchor;
    # EN: Return the callable names of this exact instance, in compiled order.
    # RU: Вернуть вызываемые имена этого экземпляра в порядке компиляции.
    scalarloadedexports: fn(anchor: Scalarloadedanchor): string;
    # EN: A nil offer binds the typed view privately, without profile I/O.
    # RU: При offer == nil связать приватное представление без ввода-вывода профиля.
    scalarloadedview: fn(anchor: Scalarloadedanchor, exportname, offer: string):
        (int, Scalarimplementation, Scalarbigimplementation, Scalartupleimplementation,
              Scalarrefimplementation, Scalarlistimplementation, Scalarrefpairimplementation);
    # EN: Bind an export to its owner gateway. A nil offer keeps it private.
    # RU: Связать экспорт со шлюзом владельца. При offer == nil связь остаётся приватной.
    scalarloadedslot: fn(anchor: Scalarloadedanchor, exportname, offer: string):
        (int, Scalarimplementation, Scalarbigimplementation, Scalartupleimplementation,
            Scalarrefimplementation, Scalarlistimplementation, Scalarrefpairimplementation, Scalarrawinvoke, int, int, Scalarmanagedinvoke);
    scalarmanagednext: fn(path: string): array of byte;
    scalarmanagederror: fn();
    # EN: Return 1 if the peer closed the current call fid, otherwise 0.
    # RU: Вернуть 1 при закрытии fid текущего вызова, иначе 0.
    callcancelled: fn(): int;

    scalarrawnext: fn(path: string): (int,big,big,big,big);
    scalarrawdispatchnext: fn(path: string): (int,big,big,big,big);
    scalarrefdispatchnext: fn(path: string): (int,int,big);
    scalarrefpairdispatchnext: fn(path: string): (int,int,big,int,big);
    scalarcallstart: fn();
    scalarcallstage: fn(worker: int);
    scalarcalltake: fn();
    scalarcalltakefrom: fn(source: int);
    acceptsharedlaunch: fn();
    scalarcallkey: fn(): (big,big,big,big,int);
    scalarcallabort: fn();
    scalarrawreply: fn(path: string, success, count: int, value: (big,big,big,big));
};
