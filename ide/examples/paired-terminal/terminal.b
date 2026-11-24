# Copyright (c) 2026 bhgv
# EN: Upload the compiled movie and check a Unicode view frame on ordinary Limbo channels.
# RU: Загрузить собранный фильм и проверить кадр Unicode на обычных каналах Limbo.

implement Terminal;
include "sys.m";
include "unibus.m";

Terminal: module {
    init: fn();
};


# EN: Send one counted movie to the attached player. The final write commits the upload.
# RU: Передать фильм с длиной подключённому плееру. Последняя запись завершает загрузку.
uploadmovie(sys: Sys)
{
    source := sys->open("terminal.swf", Sys->OREAD);
    if (source == nil) {
        raise sys->sprint("open terminal.swf: %r");
    }
    (status, entry) := sys->fstat(source);
    if (status < 0 || entry.length <= big 0 || entry.length > big 1048576) {
        raise "terminal.swf must be between 1 byte and 1 MiB";
    }
    size := int entry.length;
    upload := sys->open("/rpc/.upload", Sys->OWRITE);
    if (upload == nil) {
        raise sys->sprint("open .upload: %r");
    }
    count := array[4] of {byte (size >> 24), byte (size >> 16), byte (size >> 8), byte size};
    if (sys->write(upload, count, len count) != len count) {
        raise sys->sprint("write SWF length: %r");
    }
    buffer := array[4096] of byte;
    while (size > 0) {
        chunk := size;
        if (chunk > len buffer) {
            chunk = len buffer;
        }
        if (sys->readn(source, buffer, chunk) != chunk ||
            sys->write(upload, buffer, chunk) != chunk) {
            raise sys->sprint("transfer terminal.swf: %r");
        }
        size -= chunk;
    }
}


# EN: Exchange one view with the paired player and compare the complete reply.
# RU: Обменяться кадром с парным плеером и сравнить ответ целиком.
init()
{
    sys := load Sys Sys->PATH;
    bus := load Unibus Unibus->PATH;
    uploadmovie(sys);
    input := chan of array of byte;
    output := chan of array of byte;
    if (bus == nil || bus->attach(input, output) != 0) {
        raise "attach paired Unibus";
    }

    # On the controller scheduler, the unbuffered send precedes the next guest turn.
    # В планировщике контроллера небуферизованная отправка предшествует следующему такту программы.
    view := "fvd1:000000020000002a00000000000000000000004200000000Я64";
    output <-= array of byte view;
    state := sys->open("/rpc/.state", Sys->OWRITE);
    if (state == nil || sys->write(state, array of byte "start", 5) != 5) {
        raise sys->sprint("start terminal.swf: %r");
    }
    reply := <-input;
    bus->detach();
    if (string reply != view + " AS2") {
        raise "paired Unicode frame mismatch";
    }
    sys->print("PASS compiled AS2 + Limbo: %s\n", string reply);
}
