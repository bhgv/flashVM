# Copyright (c) 2026 bhgv
# EN: Read the scalar sample's resource from the executing owner's namespace.
# RU: Прочитать ресурс скалярного примера из пространства имён владельца.
implement Privatevalue;
include "sys.m";
include "privatevalue.m";


# EN: Open and read the local demonstration resource, raising on I/O failure.
# RU: Открыть и прочитать локальный ресурс примера, при ошибке вызвать исключение.
read(): int
{
    sys := load Sys Sys->PATH;
    file := sys->open("provider-secret", Sys->OREAD);
    if (file == nil) {
        raise "provider resource absent";
    }
    bytes := array[16] of byte;
    n := sys->read(file, bytes, len bytes);
    if (n <= 0) {
        raise "provider resource unreadable";
    }
    return int string bytes[:n];
}
