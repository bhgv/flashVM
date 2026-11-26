# Copyright (c) 2026 bhgv
# EN: Drive a stock named-channel rendezvous from the paired Limbo application.
# RU: Проверить штатный именованный канал из парного приложения Limbo.
implement Client;
include "sys.m";
include "styxnamed.m";

Client: module { init: fn(); };
sys: Sys; # Paired runtime filesystem and scheduler.
named: Styxnamed; # Limbo binding to the common native registry.
Port: import named;


# EN: Upload the compiled SWF to the paired player's control endpoint and start it.
# RU: Загрузить собранный SWF в точку управления плеера и запустить его.
start()
{
    fd := sys->open("named.swf", Sys->OREAD);
    bytes := array[8192] of byte;
    n := sys->read(fd, bytes[4:], len bytes - 4);
    if (n <= 0 || n >= len bytes - 4) {
        raise "cannot read named SWF";
    }
    bytes[2] = byte (n >> 8);
    bytes[3] = byte n;
    upload := sys->open("/peer/.upload", Sys->OWRITE);
    if (upload == nil) {
        raise sys->sprint("cannot open named SWF upload: %r");
    }
    offset := 0;
    while (offset < n+4) {
        count := n+4-offset;
        if (count > 2048) {
            count = 2048;
        }
        written := sys->write(upload, bytes[offset:], count);
        if (written <= 0) {
            raise sys->sprint("cannot upload named SWF at %d: %r", offset);
        }
        offset += written;
    }
    upload = nil;
    state := sys->open("/peer/.state", Sys->OWRITE);
    command := array of byte "start";
    if (state == nil || sys->write(state, command, len command) != len command) {
        raise sys->sprint("cannot start named SWF: %r");
    }
}



# EN: Compare wire frames byte for byte, including embedded NUL values.
# RU: Сравнить сетевые кадры побайтово, включая нулевые байты.
equal(a, b: array of byte): int
{
    if (len a != len b) {
        return 0;
    }
    for (i := 0; i < len a; ++i) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}


# EN: Return one if a real VFS file rejects enrollment as a channel.
# RU: Вернуть единицу, если обычный файл VFS нельзя открыть как канал.
collision(): int
{
    {
        port := named->open("/named.swf");
        port.close();
    } exception e {
    "*" =>
        return 1;
    }
    return 0;
}


# EN: Both peers share the exact name. Test scalar and graph frames and lease retirement.
# RU: Общий точный путь у обеих сторон. Проверить скаляры, графы и закрытие аренды.
init()
{
    sys = load Sys Sys->PATH;
    named = load Styxnamed Styxnamed->PATH;
    if (named == nil) {
        raise "cannot load Styxnamed";
    }
    named->init();
    if (!collision()) {
        raise "named channel replaced an existing file";
    }
    blocking();
    port := named->open("/jobs");
    temporary := named->open("/jobs");
    temporary.close();
    (exists, meta) := sys->stat("/jobs");
    if (exists != 0 || meta.mode != 0 || sys->open("/jobs", Sys->OREAD) != nil) {
        raise "named channel failed to reserve its VFS name";
    }
    start();
    value := array[] of {byte Styxnamed->Text, byte 'A', byte 0, byte 'B'};
    port.send(value);
    if (!equal(port.recv(), value)) {
        raise "named channel scalar frame differs";
    }
    graph := port.recv();
    if (len graph < 2 || int graph[0] != Styxnamed->Snapshot) {
        raise "named channel array is not a snapshot";
    }
    port.send(graph);
    if (!equal(port.recv(), graph)) {
        raise "named channel array frame changed on decode and encode";
    }
    port.close();
    for (i := 0; i < 100; ++i) {
        (exists, meta) = sys->stat("/jobs");
        if (exists < 0) {
            sys->print("PASS native named RPC paired VMs\n");
            return;
        }
        sys->sleep(10);
    }
    raise "last close did not release the name";
}


# EN: Write one frame from an independent Dis thread and report its return.
# RU: Записать один кадр из отдельного потока Dis и сообщить о возврате.
send(port: ref Port, sent: chan of int)
{
    port.send(array[] of {byte Styxnamed->Boolean, byte 1});
    sent <-= 1;
}


# EN: A write must still be waiting until the other participant requests a read.
# RU: Запись должна ждать, пока другой участник не запустит чтение.
blocking()
{
    writer := named->open("/waiting");
    reader := named->open("/waiting");
    sent := chan[1] of int;
    spawn send(writer, sent);
    sys->sleep(50);
    alt {
    <-sent =>
        raise "named channel acknowledged before a read";
    * =>
        ;
    }
    frame := reader.recv();
    if (len frame != 2 || int frame[0] != Styxnamed->Boolean || int frame[1] != 1 || <-sent != 1) {
        raise "named channel rendezvous changed the sent value";
    }
    writer.close();
    reader.close();
}
