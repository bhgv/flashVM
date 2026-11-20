# Copyright (c) 2026 bhgv
# EN: Typed convenience API for scalar native layer snapshots and edits.
# RU: Типизированный API поверх scalar native layer snapshots и edits.
implement Vfsctl;
include "vfsctl.m";
include "vfsnative.m";
native: Vfsnative; # Immutable service reference after init.


# EN/RU: Initialize before callers share the module instance.
# Инициализировать до sharing module instance; raise on missing builtin.
init()
{
    native = load Vfsnative Vfsnative->PATH;
    if (native == nil) {
        raise "Vfsctl requires native layer service";
    }
}


# EN/RU: Decode one bounded UTF-8 field at a fixed config revision.
# Читать bounded UTF-8 field по fixed revision; status/nil on failure.
text(path: string, revision: big, index, part: int): (int,string)
{
    bytes := array[4096] of byte;
    for (i := 0; i < len bytes; i++) {
        (status, value) := native->text(path, revision, index, part, i);
        if (status == 0) {
            return (Ready, string bytes[:i]);
        }
        if (status != Ready) {
            return (status, nil);
        }
        bytes[i] = byte value;
    }
    return (Error, nil);
}


# EN/RU: Combine copied card/text/source state, refusing mixed config revisions.
# Объединить card/text/state; mixed config revisions отвергаются без partial Layer.
layer(path: string, revision: big, index: int): (int,big,ref Layer)
{
    (status, version, head, id, count, position, flags, kind, metadata, mode, directory, access) :=
        native->layer(path, revision, index);
    if (status != Ready) {
        return (status, version, nil);
    }
    mount, relative: string;
    (status, mount) = text(path, version, index, 0);
    if (status != Ready) {
        return (status, version, nil);
    }
    (status, relative) = text(path, version, index, 1);
    if (status != Ready) {
        return (status, version, nil);
    }
    session, generation: big;
    capabilities: int;
    (status, session, generation, capabilities) = native->state(path, version, index);
    if (status != Ready) {
        return (status, version, nil);
    }
    return (Ready, version, ref Layer(id, head, position, count, flags, kind, metadata, mode,
        directory, access, mount, relative, session, generation, capabilities));
}


# EN/RU: Reverse a private list to restore top-to-bottom traversal order.
# Перевернуть private list, сохраняя Layer ownership в managed references.
reverse(input: list of ref Layer): list of ref Layer
{
    result: list of ref Layer;
    for (; input != nil; input = tl input) {
        result = hd input :: result;
    }
    return result;
}


# EN/RU: Capture finite effective stack; retry config races at most8 times.
# Снять finite effective stack; config races повторять не более8 раз.
layers(path: string): (int,big,list of ref Layer)
{
    for (attempt := 0; attempt < 8; attempt++) {
        (status, version, first) := layer(path, big -1, 0);
        if (status == Stale) {
            continue;
        }
        if (status != Ready) {
            return (status, version, nil);
        }
        result: list of ref Layer;
        result = first :: result;
        count := first.count;
        for (i := 1; i < count; i++) {
            current: ref Layer;
            (status, nil, current) = layer(path, version, i);
            if (status != Ready) {
                break;
            }
            result = current :: result;
        }
        if (status == Stale) {
            continue;
        }
        if (status != Ready) {
            return (Error, version, nil);
        }
        (status, nil, nil, nil) = native->state(path, version, 0);
        if (status == Stale) {
            continue;
        }
        if (status == Ready) {
            return (Ready, version, reverse(result));
        }
        return (Error, version, nil);
    }
    return (Stale, big 0, nil);
}


# EN/RU: Mutate once; never repeat a successful native effect on result publication.
# Выполнить mutation один раз; без replay уже выполненного native effect.
bind(source, target: string, flags, index: int): int
{
    return native->bind(source, target, flags, index);
}


# EN/RU: Require caller's observed version and binding ID, scoped to this namespace.
# Требовать observed revision/ID своего namespace; mutation выполняется один раз.
move(path: string, revision, id: big, index: int): int
{
    return native->move(path, revision, id, index);
}



# EN/RU: Copy source text at one caller-supplied revision without metadata or rights callbacks.
# Скопировать provenance по заданной revision без metadata/rights callbacks.
origin(path: string, revision: big, index: int): (int,ref Origin)
{
    if (revision < big 0) {
        return (Error, nil);
    }
    values := array[5] of string;
    for (i := 0; i < len values; i++) {
        (status, value) := text(path, revision, index, i+2);
        if (status != Ready) {
            return (status, nil);
        }
        values[i] = value;
    }
    if (values[0] == "") {
        return (Ready, nil);
    }
    return (Ready, ref Origin(int values[0], values[1], values[2], values[3], values[4]));
}


# EN/RU: Submit the named VFS program from the caller's namespace.
# Передать запуск указанной программы из пространства имён вызывающего.
run(path: string): int
{
    return native->run(path);
}


# EN/RU: Send binary startup data to the selected command route.
# Отправить бинарные стартовые данные по маршруту команды.
runargs(path: string, arguments: array of byte): int
{
    return native->runargs(path, arguments);
}
