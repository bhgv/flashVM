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
    check_refs(m);
    check_nested(m);
    check_returned_refs(m);
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

# EN: Verify aliased copy-back, dirty suppression and rollback on owner failure.
# RU: Проверить обновление общей ссылки, пропуск прежних значений и отказ после ошибки.
check_refs(m: Managed)
{
    Cell: import m;
    bytes := array[2500] of byte;
    bytes[0] = byte 12;
    bytes[1] = byte 90;
    bytes[len bytes - 1] = byte 90;
    held := array[2500] of byte;
    held[2499] = byte 234;
    cell := ref Cell("x", bytes);
    other := ref Cell("stable", held);
    if (m->update("noop", cell, cell, other) != "stable" ||
        cell.bytes != bytes || other.bytes != held) {
        raise "unchanged refs were overwritten";
    }
    if (m->update("text", cell, cell, other) != "x!" ||
        cell.text != "x!" || cell.bytes != bytes || other.bytes != held) {
        raise "text update replaced an untouched array";
    }
    if (m->update("write", cell, cell, other) != "x!!" ||
        cell.text != "x!!" || cell.bytes[1] != byte 91 ||
        cell.bytes[len cell.bytes - 1] != byte 92 || other.bytes != held) {
        raise "sparse aliased ref update failed";
    }
    if (m->update("grow", cell, cell, other) != "x!!" || len cell.bytes != 2501 ||
        cell.bytes[2499] != byte 92 || cell.bytes[2500] != byte 77 || other.bytes != held) {
        raise "array append via ref failed";
    }
    if (m->update("shrink", cell, cell, other) != "x!!" || len cell.bytes != 2500 ||
        cell.bytes[2499] != byte 92 || other.bytes != held) {
        raise "array truncation via ref failed";
    }
    failed := 0;
    {
        m->update("fail", cell, cell, other);
    } exception {
        "*" => failed = 1;
    }
    if (!failed || cell.text != "x!!" || cell.bytes[1] != byte 91 ||
        cell.bytes[len cell.bytes - 1] != byte 92 || other.bytes != held) {
        raise "failed call modified client refs";
    }
    if (m->update("mixed", cell, cell, other) != "x!!" || len cell.bytes != 2501 ||
        cell.bytes[0] != byte 79 || cell.bytes[2] != byte 91 || cell.bytes[2500] != byte 98 ||
        other.bytes != held) {
        raise "disjoint array insertion and replacement via ref failed";
    }
    text := "é";
    for (i := 0; i < 10; i++) {
        text = text + text;
    }
    cell.text = text;
    prior := cell.bytes;
    if (m->update("utf8", cell, cell, other) != "é🌍" ||
        cell.text != "é🌍" + text[2:] || cell.bytes != prior) {
        raise "UTF-8 text delta via ref failed";
    }

}


# EN: Verify both value return and recursive ref write-back after one ordinary load.
# RU: Проверить возврат по значению и вложенное обновление ref после обычного load.
check_nested(m: Managed)
{
    Item, Nested: import m;
    names := array[] of {"é", nil, "🌍"};
    rows := array[3] of array of byte;
    rows[0] = array[] of {byte 42};
    rows[2] = array[0] of byte;
    items := array[400] of Item;
    for (i := 0; i < len items; i++) {
        items[i] = (byte (i % 200), "stable");
    }
    items[0] = (byte 9, "old");
    state := ref Nested(items, rows);
    alias := state;
    (returned, records) := m->nested(names, state);
    (code, label) := records[0];
    (updated, updatedlabel) := alias.items[0];
    (last, lastlabel) := records[len records - 1];
    if (returned[0] != "é!" || returned[1] != nil || returned[2] != "🌍" ||
        code != byte 10 || label != "new" || updated != code || updatedlabel != label ||
        len records != 400 || last != byte 199 || lastlabel != "stable" ||
        alias.rows[0][0] != byte 99 || alias.rows[1] != nil ||
        alias.rows[2] == nil || len alias.rows[2] != 0 ||
        names[0] != "é" || rows[0][0] != byte 42) {
        raise "nested value or ref update failed";
    }
}


# EN: Observe returned aliases and copy-back on the original client object.
# RU: Проверить алиасы в результате и обновление исходного объекта клиента.
check_returned_refs(m: Managed)
{
    Cell: import m;
    original := ref Cell("old", nil);
    (existing, created, repeated, absent) := m->reference(original);
    if (existing != original || existing.text != "updated" ||
        created == nil || created == original || created != repeated || absent != nil ||
        created.text != "fresh" || len created.bytes != 1 || created.bytes[0] != byte 31) {
        raise "remote ref result lost object identity";
    }
    repeated.text = "client";
    if (created.text != "client") {
        raise "fresh returned aliases are not shared";
    }
}
