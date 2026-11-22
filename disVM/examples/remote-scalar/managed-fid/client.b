# Copyright (c) 2026 bhgv
# EN: Exercise managed values, exceptions and parallel calls over a Styx mount.
# RU: Проверка управляемых значений, исключений и параллельных вызовов через Styx.
implement Client;
include "sys.m";
include "managed.m";
Client: module { init: fn(); };
# EN: Load the mounted library and validate normal and exceptional calls.
# RU: Загрузить смонтированную библиотеку и проверить возврат и исключение.
init()
{
    sys := load Sys Sys->PATH;
    m := load Managed "/transport/mounted/managed.dis";
    if (m == nil) {
        raise "load managed: " + sys->sprint("%r");
    }
    a := array[] of {byte 42, byte 0, byte 250};
    (text, b) := m->echo("Ж", a);
    if (text != "Ж!" || b[1] != byte 99 || b[2] != byte 250 || a[1] != byte 0) {
        raise "bad managed return";
    }
    m->accept("ok");
    rejected := 0;
    {
        m->accept("bad");
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected) {
        raise "missing managed error";
    }
    m->accept("ok");
    done := chan of int;
    spawn large(m, "first", done);
    spawn large(m, "second", done);
    first := <-done;
    second := <-done;
    if (!first || !second) {
        raise "parallel managed calls lost arguments or results";
    }
    sys->print("PASS managed Styx load\n");
}


# EN: Send a record larger than a Styx message and check the complete returned array.
# RU: Отправить запись длиннее сообщения Styx и проверить весь возвращённый массив.
large(m: Managed, label: string, done: chan of int)
{
    {
        bytes := array[5000] of byte;
        bytes[0] = byte 42;
        bytes[len bytes - 1] = byte 255;
        (text, reply) := m->echo(label, bytes);
        done <-= text == label + "!" && len reply == len bytes &&
            reply[1] == byte 99 && reply[len reply - 1] == byte 255 && bytes[1] == byte 0;
    } exception {
        "*" =>
            done <-= 0;
    }
}
