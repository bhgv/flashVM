# Copyright (c) 2026 bhgv
# EN: Call executable VFS nodes through an existing Styx mount.
# RU: Вызов исполняемых узлов VFS через действующее монтирование Styx.
StyxexecHandler: module
{
    # EN: Process the request in the calling Limbo owner and return reply bytes.
    # RU: Обработать запрос в вызывающем потоке Limbo и вернуть байты ответа.
    invoke: fn(path: string, bytes: array of byte): array of byte;
};

Styxexec: module
{
    PATH: con "styxexec.dis"; # Installed portable implementation.
    Maxpath: con 1024; # UTF-8 path bytes, excluding the terminal zero.
    Maxbytes: con 65536; # Argument bytes and maximum reply length.

    Request: adt
    {
        ticket: big; # Opaque locally scoped reply correlation.
        bytes: array of byte; # Owned copy of the request.
    };

    Node: adt
    {
        fd: ref Sys->FD; # Owned registration, removed on close.
        wire: array of byte; # Private reusable mailbox read buffer.
        path: string; # Immutable absolute publication path used by dispatch.
        # EN: Wait for one request on the node's owner thread.
        # RU: Дождаться одного запроса в потоке владельца узла.
        recv: fn(node: self ref Node): ref Request;
        # EN: Answer request with a byte prefix no longer than its original input.
        # RU: Ответить на request префиксом байтов не длиннее исходного входа.
        reply: fn(node: self ref Node, request: ref Request, bytes: array of byte): int;
        # EN: Refuse this exact request; return zero if it already expired.
        # RU: Отказать в точном запросе. Вернуть ноль, если он уже истёк.
        refuse: fn(node: self ref Node, request: ref Request): int;
        # EN: Receive one request, invoke a borrowed handler and reply or refuse.
        # RU: Принять один запрос, вызвать обработчик и ответить или отказать.
        dispatch: fn(node: self ref Node, handler: StyxexecHandler): int;
        # EN: Release the VFS name and reject an open request. Repeated close is harmless.
        # RU: Освободить имя VFS и отклонить запрос. Повторное закрытие допустимо.
        close: fn(node: self ref Node);
    };

    # EN: Publish an execute-only node in the paired DisVM namespace.
    # RU: Опубликовать узел только на исполнение в пространстве парной DisVM.
    publish: fn(path: string): ref Node;

    # EN: Load Sys before calls.
    # RU: Загрузить Sys перед вызовами.
    init: fn();

    # EN: Invoke a path relative to an absolute Styx mount. Borrow input and
    # return an owned byte prefix. Waits for the callback and raises on I/O errors.
    # RU: Вызвать путь относительно абсолютной точки монтирования Styx.
    # Заимствует вход и возвращает собственный префикс байтов.
    # Ожидает колбэк. Ошибки I/O дают исключение.
    call: fn(mount, path: string, input: array of byte): array of byte;
};
