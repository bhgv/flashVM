# Copyright (c) 2026 bhgv
# EN: Original Dis implementation exercised through the mounted library.
# RU: Исходная реализация Dis для вызовов через смонтированную библиотеку.
implement Managed;
include "managed.m";
include "sys.m";
# EN: Mutate the locally decoded byte array and return it with the decorated text.
# RU: Изменить локальный массив байтов и вернуть его вместе с дополненной строкой.
echo(text: string, bytes: array of byte): (string, array of byte)
{
    if (len bytes < 3 || bytes[0] != byte 42) {
        raise "invalid test argument";
    }
    bytes[1] = byte 99;
    return (text + "!", bytes);
}
# EN: Complete without a value or raise the requested exception.
# RU: Завершиться без значения либо вызвать исключение по запросу.
accept(text: string)
{
    if (text != "ok") {
        raise "rejected";
    }
}


# EN: Change the aliased ADT once, touch an unchanged field, or raise after a write.
# RU: Изменить общий ADT, записать прежнее значение либо вызвать исключение после записи.
update(mode: string, left, right, untouched: ref Cell): string
{
    if (left != right) {
        raise "remote aliases diverged";
    }
    untouched.text = untouched.text;
    if (mode == "noop") {
        return untouched.text;
    }
    if (mode == "utf8") {
        left.text = left.text[:1] + "🌍" + left.text[2:];
        return right.text[:2];
    }
    if (mode == "mixed") {
        grown := array[len left.bytes + 1] of byte;
        grown[0] = byte 79;
        grown[1:] = left.bytes;
        grown[len grown - 1] = byte 98;
        left.bytes = grown;
        return right.text;
    }
    if (mode == "grow") {
        grown := array[len left.bytes + 1] of byte;
        grown[0:] = left.bytes;
        grown[len grown - 1] = byte 77;
        left.bytes = grown;
        return right.text;
    }
    if (mode == "shrink") {
        left.bytes = left.bytes[:len left.bytes - 1];
        return right.text;
    }
    left.text = left.text + "!";
    if (mode != "text") {
        right.bytes[1] = byte 91;
        right.bytes[len right.bytes - 1] = byte 92;
    }
    if (mode == "fail") {
        raise "discard ref updates";
    }
    return right.text;
}


# EN: Exercise nested S-records and write reconstructed values back through ref.
# RU: Проверить вложенные S-записи и вернуть восстановленные значения через ref.
nested(names: array of string, state: ref Nested): (array of string, array of Item)
{
    if (len names != 3 || names[0] != "é" || names[1] != nil || names[2] != "🌍" ||
        len state.rows != 3 || state.rows[1] != nil || state.rows[2] == nil ||
        len state.rows[2] != 0 || state.rows[0][0] != byte 42) {
        raise "bad nested request";
    }
    (code, label) := state.items[0];
    if (code != byte 9 || label != "old") {
        raise "bad tuple element";
    }
    names[0] = names[0] + "!";
    state.items[0] = (byte 10, "new");
    state.rows[0][0] = byte 99;
    return (names, state.items);
}


# EN: Return an existing object and two aliases of one fresh object.
# RU: Вернуть существующий объект и две ссылки на один новый объект.
reference(state: ref Cell): (ref Cell, ref Cell, ref Cell, ref Cell)
{
    state.text = "updated";
    fresh := ref Cell("fresh", array[] of {byte 31});
    return (state, fresh, fresh, nil);
}


# EN: Share both an input and a newly created object inside the returned graph.
# RU: Разделить исходный и новый объекты между полями возвращаемого графа.
bundle(state: ref Cell): ref Bundle
{
    state.text = "nested update";
    fresh := ref Cell("member", nil);
    members := array[258] of ref Cell;
    members[0] = state;
    members[1] = fresh;
    for (i := 3; i < 256; i++)
        members[i] = ref Cell("item", nil);
    members[256] = members[255];
    members[257] = fresh;
    return ref Bundle(fresh, members);
}


# EN: Update a shared child after removing its original paths from the provider's graph.
# RU: Изменить общий дочерний объект после удаления исходных путей к нему у владельца.
changegraph(state: ref Bundle, extra: array of ref Cell, fail: int): (ref Cell, array of byte)
{
    old := state.direct;
    if (old != state.members[0] || old != extra[0] || state.members[1] != nil) {
        raise "nested arguments lost shared identity";
    }
    state.direct = ref Cell("fresh", nil);
    state.members[0] = state.direct;
    extra[31].text = "last index";
    extra[0] = nil;
    old.text = "changed after unlink";
    if (fail == 2) {
        sys := load Sys Sys->PATH;
        sys->sleep(3000);
    } else if (fail == 3) {
        check_chain_context();
    } else if (fail) {
        raise "graph failed after mutation";
    }
    output := array[3000] of byte;
    output[2999] = byte 53;
    return (old, output);
}

# EN: Follow the VFS and FD that B inherited from A.
# RU: Обратиться к VFS и дескриптору, унаследованным B от A.
check_chain_context()
{
    sys := load Sys Sys->PATH;
    marker := sys->open("/.prog/.self/.vfs/chain-stage", Sys->OREAD);
    output := sys->open("/.prog/.self/.fd/8", Sys->ORDWR);
    data := array[4] of byte;
    if (marker == nil || output == nil)
        raise "chain library lost caller context";
    n := sys->read(marker, data, len data);
    if (n != 4 || data[0] != byte 0 || data[1] != byte 0 || data[2] != byte 0 || data[3] != byte 42)
        raise "chain library lost the delegated VFS";
    data[0] = byte 'D';
    if (sys->write(output, data, 1) != 1)
        raise "chain library lost inherited FD";
}
