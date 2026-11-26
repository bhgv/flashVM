# Copyright (c) 2026 bhgv
# EN: Call executable VFS nodes through an existing Styx mount.
# RU: Вызов исполняемых узлов VFS через действующее монтирование Styx.
Styxexec: module
{
    PATH: con "styxexec.dis"; # Installed portable implementation.
    Maxpath: con 1024; # UTF-8 path bytes, excluding the terminal zero.
    Maxbytes: con 65536; # Argument bytes and maximum reply length.

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
