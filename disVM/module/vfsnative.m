# Copyright (c) 2026 bhgv
# EN: Scalar native bridge for caller-namespace layer control; prefer Vfsctl wrapper.
# RU: Scalar native bridge управления слоями своего namespace; удобный wrapper Vfsctl.
Scalarimplementation: module
{
    invoke: fn(value: int): int; # Single linked slot; the native loader selects its export.
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
    # EN/RU: Submit the mounted program by its visible path;1 delivered,0 refused,-1 uncertain.
    # Отправить запуск по видимому пути;1 доставлено,0 отказ,-1 исход неизвестен.
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
};
