# Copyright (c) 2026 bhgv
# EN: Validate the portable data graph and encode it with the current FVMG format.
# RU: Проверять переносимые данные графа и записывать их в текущем формате FVMG.
Lengthgetter: con 600; # Stable FlashVM Array length callback id.
Lengthsetter: con 601; # Stable FlashVM Array resize callback id.
Lengthflags: con 3; # Non-enumerable, non-deletable Array length.

GraphWriter: adt
{
    bytes: array of byte; # One owned buffer bounded by the named frame limit.
    at: int; # Next byte position after initialized output.
};


# EN: Enqueue the target of a link once; reject nested snapshot packets as atoms.
# RU: Добавить цель ссылки один раз. Вложенные пакеты снимка недопустимы как значения.
visit(item: ref Value, graph: ref DataGraph, ids, order: array of int, used: int): int
{
    if (item == nil) {
        raise "missing named graph value";
    }
    pick edge := item {
    Link =>
        index := edge.index;
        if (index < 0 || index >= len graph.nodes) {
            raise "invalid named graph link";
        }
        if (graph.nodes[index] == nil) {
            raise "named graph data profile";
        }
        if (ids[index] == Gnone) {
            ids[index] = used;
            order[used++] = index;
        }
    Graph =>
        raise "nested named graph envelope";
    }
    return used;
}


# EN: Validate unique named keys without reordering the stored members.
# RU: Проверить уникальность имён без изменения порядка полей.
checkfields(fields: array of Field)
{
    if (len fields > Maxframe) {
        raise "too many named graph fields";
    }
    used := array[2*len fields+1] of int;
    for (i := 0; i < len used; ++i) {
        used[i] = 0;
    }
    for (i = 0; i < len fields; ++i) {
        name := fields[i].name;
        bucket := 0;
        for (j := 0; j < len name; ++j) {
            bucket = (bucket*31 + name[j]) % len used;
        }
        while (used[bucket] != 0) {
            if (fields[used[bucket]-1].name == name) {
                raise "duplicate named graph field";
            }
            bucket = (bucket+1) % len used;
        }
        used[bucket] = i+1;
    }
}


# EN: Validate unique sparse keys and the kind-specific UInt32 range.
# RU: Проверить уникальные разреженные ключи и диапазон UInt32 для вида объекта.
checkslots(node: ref Node)
{
    slots := node.slots;
    if (len slots > Maxframe) {
        raise "too many named graph slots";
    }
    used := array[2*len slots+1] of int;
    for (i := 0; i < len used; ++i) {
        used[i] = 0;
    }
    for (i = 0; i < len slots; ++i) {
        key := slots[i].key;
        if (key < big 0 || key > Uintword || (node.kind != Tablekind && key == Uintword)) {
            raise "invalid named graph index";
        }
        bucket := int ((key ^ (key >> 16)) % big len used);
        while (used[bucket] != 0) {
            if (slots[used[bucket]-1].key == key) {
                raise "duplicate named graph index";
            }
            bucket = (bucket+1) % len used;
        }
        used[bucket] = i+1;
    }
}


# EN: Return compact ids, traversal order, and the number of reachable nodes.
# RU: Вернуть компактные номера, порядок обхода и число достижимых узлов.
reachable(graph: ref DataGraph): (array of int, array of int, int)
{
    if (graph == nil || len graph.nodes > Gobjects) {
        raise "invalid named graph";
    }
    ids := array[len graph.nodes] of int;
    order := array[len graph.nodes] of int;
    for (i := 0; i < len ids; ++i) {
        ids[i] = Gnone;
    }
    used := visit(graph.root, graph, ids, order, 0);
    for (i = 0; i < used; ++i) {
        node := graph.nodes[order[i]];
        if (node.kind != Tablekind && node.kind != Arraykind && node.kind != Objectkind) {
            raise "named graph data profile";
        }
        checkfields(node.fields);
        checkslots(node);
        for (j := 0; j < len node.fields; ++j) {
            used = visit(node.fields[j].value, graph, ids, order, used);
        }
        for (j = 0; j < len node.slots; ++j) {
            used = visit(node.slots[j].value, graph, ids, order, used);
        }
    }
    return (ids, order, used);
}


# EN: Reserve space without extending the maximum frame size.
# RU: Зарезервировать место в пределах размера кадра.
reserve(w: ref GraphWriter, size: int): int
{
    if (size < 0 || size > len w.bytes-w.at) {
        raise "named graph is too long";
    }
    at := w.at;
    w.at += size;
    return at;
}


# EN: Write the low byte of value.
# RU: Записать младший байт value.
putbyte(w: ref GraphWriter, value: int)
{
    w.bytes[reserve(w, 1)] = byte value;
}


# EN: Write one word at a previously reserved position in big-endian order.
# RU: Записать слово на зарезервированном месте старшим байтом вперёд.
patchword(bytes: array of byte, at, value: int)
{
    for (i := 3; i >= 0; --i) {
        bytes[at+i] = byte value;
        value >>= 8;
    }
}


# EN: Append a complete big-endian word.
# RU: Добавить слово старшим байтом вперёд.
putword(w: ref GraphWriter, value: int)
{
    patchword(w.bytes, reserve(w, 4), value);
}


# EN: Append a counted copy of bytes.
# RU: Добавить копию байтов с длиной.
puttext(w: ref GraphWriter, bytes: array of byte)
{
    putword(w, len bytes);
    at := reserve(w, len bytes);
    if (len bytes) {
        w.bytes[at:] = bytes;
    }
}


# EN: Emit a scalar or a compact object link, with no primitive metadata.
# RU: Записать скаляр или компактную ссылку без метаданных примитива.
putvalue(w: ref GraphWriter, item: ref Value, ids: array of int)
{
    pick value := item {
    Numeric =>
        putbyte(w, Gnumber);
        bits := math->realbits64(value.value);
        for (i := (Wordbytes-1)*8; i >= 0; i -= 8) {
            putbyte(w, int (bits>>i));
        }
    Textual =>
        putbyte(w, Gtext);
        puttext(w, value.bytes);
    Logical =>
        if (value.value != 0 && value.value != 1) {
            raise "invalid graph Boolean";
        }
        putbyte(w, Gbool);
        putbyte(w, value.value);
    Nullval =>
        putbyte(w, Gnull);
    Undefinedval =>
        putbyte(w, Gundefined);
    Link =>
        putbyte(w, Glink);
        putword(w, ids[value.index]);
    Graph =>
        raise "nested named graph envelope";
    }
    putword(w, Gnone);
}


# EN: Write one built-in accessor id or one absent callback.
# RU: Записать идентификатор встроенного аксессора либо отсутствие колбэка.
putcallback(w: ref GraphWriter, id: int)
{
    putword(w, id);
    putword(w, 0);
    putword(w, Gnone);
    putbyte(w, 0);
}


# EN: Encode one reachable data record. Sparse numeric and named keys stay separate.
# RU: Закодировать одну достижимую запись. Числовые и именованные ключи раздельны.
putnode(w: ref GraphWriter, node: ref Node, ids: array of int)
{
    putword(w, node.kind);
    arraynode := node.kind == Arraykind;
    if (node.length < big 0 || node.length > Uintword || (!arraynode && node.length != big 0)) {
        raise "invalid named array length";
    }
    if (arraynode) {
        putvalue(w, ref Value.Numeric(real node.length), ids);
    } else {
        putvalue(w, ref Value.Undefinedval(0), ids);
    }
    putword(w, Gnone); # Prototype.
    putword(w, Gnone); # Metatable.
    putword(w, 0); # Interface edges.
    putword(w, len node.fields);
    for (i := 0; i < len node.fields; ++i) {
        field := node.fields[i];
        if (arraynode && field.name == "length") {
            raise "array length is stored in the graph node";
        }
        puttext(w, array of byte field.name);
        putvalue(w, field.value, ids);
    }
    putword(w, len node.slots);
    for (i = 0; i < len node.slots; ++i) {
        slot := node.slots[i];
        putword(w, int slot.key);
        putvalue(w, slot.value, ids);
    }
    putword(w, 0); # Methods.
    putword(w, 0); # Metamethods.
    putword(w, arraynode); # Accessors.
    if (arraynode) {
        puttext(w, array of byte "length");
        putcallback(w, Lengthgetter);
        putcallback(w, Lengthsetter);
    }
    putword(w, arraynode); # Attributes.
    if (arraynode) {
        puttext(w, array of byte "length");
        putword(w, Lengthflags);
    }
    putcallback(w, 0); # Callable.
    putcallback(w, 0); # Constructor.
    putword(w, 0); # Cached methods.
    putword(w, 0); # Watchers.
}


# EN/RU: See styxvalue.m. / См. styxvalue.m.
build(graph: ref DataGraph): ref Value
{
    (ids, order, used) := reachable(graph);
    w := ref GraphWriter(array[Maxframe-1] of byte, Graphheader);
    w.bytes[0:] = array[] of {byte 'F', byte 'V', byte 'M', byte 'G', byte 0, byte 0, byte 0, byte Gbase};
    putvalue(w, graph.root, ids);
    putword(w, used);
    for (i := 0; i < used; ++i) {
        putnode(w, graph.nodes[order[i]], ids);
    }
    patchword(w.bytes, Graphsize, w.at-Graphheader);
    patchword(w.bytes, Graphcrc, int crc(w.bytes[Graphheader:w.at]));
    return ref Value.Graph(w.bytes[:w.at]);
}
