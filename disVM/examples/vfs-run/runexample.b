# Copyright (c) 2026 bhgv
# EN: Submit one command by VFS path, send binary data, and read its exit status.
# RU: Отправить команду по пути VFS, передать двоичные данные и прочитать код завершения.
implement Runexample;
include "sys.m";
include "vfsctl.m";
include "runexample.m";


# EN/RU: Decide whether the owner acknowledged delivery, preserving uncertainty.
# Проверить подтверждение владельца и отдельно сообщить о неизвестном исходе.
delivered(status: int)
{
    if (status < 0) {
        raise "launch outcome unknown";
    }
    if (status == 0) {
        raise "launch refused";
    }
}


# EN/RU: Read a newline-terminated decimal exit code with ordinary Sys operations.
# Прочитать десятичный код завершения с переводом строки обычными операциями Sys.
exitstatus(sys: Sys, path: string): int
{
    fd := sys->open(path, Sys->OREAD);
    if (fd == nil) {
        raise "cannot open command result";
    }
    data := array[1] of byte;
    status := 0;
    for (digits := 0; digits <= 3; digits++) {
        if (sys->read(fd, data, 1) != 1) {
            raise "incomplete command result";
        }
        value := int data[0];
        if (value == '\n' && digits > 0) {
            return status;
        }
        if (value < '0' || value > '9') {
            raise "invalid command result";
        }
        status = 10 * status + value - '0';
    }
    raise "oversized command result";
}


# EN/RU: Use Vfsctl through the normal module loader within a prepared namespace.
# Использовать Vfsctl через обычную загрузку модулей в подготовленном пространстве имён.
execute(command, payloadroute, result: string, payload: array of byte): int
{
    sys := load Sys Sys->PATH;
    vfs := load Vfsctl "/vfsctl.dis";
    if (sys == nil || vfs == nil) {
        raise "missing Sys or Vfsctl module";
    }
    vfs->init();
    if (payloadroute == command) {
        delivered(vfs->runargs(command, payload));
    } else {
        delivered(vfs->run(command));
        if (payloadroute != nil) {
            delivered(vfs->runargs(payloadroute, payload));
        }
    }
    return exitstatus(sys, result);
}
