# Copyright (c) 2026 bhgv
# EN: Publish a callback-backed file in the paired DisVM namespace.
# RU: Публиковать файл с колбэком в пространстве имён парной DisVM.
StyxnodeHandler: module
{
    # EN: Handle one operation and return the byte count, or a negative value to refuse.
    # RU: Обработать операцию и вернуть число байтов либо отрицательное значение для отказа.
    transfer: fn(path, operation: string, buffer: array of byte): int;
};

# EN: Handler for fixed-length files. Offset is zero on execute.
# RU: Обработчик файлов фиксированной длины. При исполнении смещение нулевое.
StyxnodePositionedHandler: module
{
    transfer: fn(path, operation: string, buffer: array of byte, offset: big): int;
};

Styxnodes: module
{
    PATH: con "styxnodes.dis"; # Installed Limbo implementation.
    Maxpath: con 1023; # Maximum absolute UTF-8 path in bytes.
    Maxbytes: con 65536; # Maximum bytes in one callback request.

    Request: adt
    {
        ticket: big; # Exact local mailbox ticket; leave unchanged.
        operation: string; # One of "r", "w", "x"; leave unchanged.
        buffer: array of byte; # Owned zeroed read capacity or a copy of the write/execute input.
    };

    Node: adt
    {
        fd: ref Sys->FD; # Owns the live publication.
        wire: array of byte; # Reusable private receive buffer.
        path: string; # Absolute path passed to handlers.

        # EN: Receive one request on the owning thread.
        # RU: Принять один запрос в потоке-владельце.
        recv: fn(node: self ref Node): ref Request;

        # EN: Reply with the accepted byte count and the buffer prefix for read/execute.
        # RU: Ответить числом принятых байтов и префиксом буфера для чтения или исполнения.
        reply: fn(node: self ref Node, request: ref Request, count: int): int;

        # EN: Reject one exact request, including one the application cannot process.
        # RU: Отклонить один точный запрос, в том числе при ошибке приложения.
        refuse: fn(node: self ref Node, request: ref Request): int;

        # EN: Receive, run the borrowed handler synchronously and report whether the reply arrived.
        # RU: Принять запрос, синхронно вызвать обработчик и сообщить, принят ли ответ.
        dispatch: fn(node: self ref Node, handler: StyxnodeHandler): int;

        # EN: Withdraw the publication and release the registration fd.
        # RU: Снять публикацию и освободить fd регистрации.
        close: fn(node: self ref Node);
    };

    # EN: Load Sys before publishing nodes.
    # RU: Загрузить Sys перед публикацией узлов.
    init: fn();

    # EN: Reserve an absent absolute path with a nonempty subset of "rwx"; raises on failure.
    # RU: Занять свободный абсолютный путь с непустым подмножеством "rwx"; при ошибке исключение.
    publish: fn(path, permissions: string): ref Node;

    # EN: Publish as an offset-aware file with a fixed byte extent.
    # RU: Опубликовать файл фиксированной длины с поддержкой смещений.
    publishat: fn(path, permissions: string, length: big): ref Node;

    # EN: Receive an owned request and its absolute byte offset (zero for execute).
    # RU: Принять собственный запрос и абсолютное смещение в байтах (ноль для исполнения).
    recvat: fn(node: ref Node): (ref Request, big);

    # EN: Serve a positioned request on the owner's thread using the borrowed module.
    # RU: Обработать запрос со смещением в потоке владельца через заимствованный модуль.
    dispatchat: fn(node: ref Node, handler: StyxnodePositionedHandler): int;
};
