# Copyright (c) 2026 bhgv
# EN: Read data entries and object links from an existing FVMG snapshot.
# RU: Читать записи данных и связи объектов из существующего снимка FVMG.
Gundefined, Gnull, Gbool, Gnumber, Gtext, Glink: con iota; # FVMG value tags.
Gbase: con 8; # FVMG with counted member keys.
Ginterfaces: con 9; # FVMG with an explicit interface mode.
Gnone: con -1; # Absent object link.
Gobjects: con 4096; # FlashVM graph codec object limit.
Gtextfield: con 1002; # Native payload layout outside this data view.
Callbacks, Accessors, Attributes, Storedvalues: con iota; # Shapes of named metadata records.

GraphReader: adt
{
    bytes: array of byte; # Borrowed envelope during inspection.
    at: int; # Next byte offset.
    version: int; # Validated envelope version.
};


# EN: Take a bounded consecutive view and advance the reader.
# RU: Взять ограниченный последовательный срез и продвинуть позицию.
scan(r: ref GraphReader, size: int): array of byte
{
    if (size < 0 || size > len r.bytes-r.at) {
        raise "truncated named graph";
    }
    start := r.at;
    r.at += size;
    return r.bytes[start:r.at];
}


# EN: Read one big-endian graph word as int bits.
# RU: Прочитать слово графа старшим байтом вперёд как биты int.
gword(r: ref GraphReader): int
{
    return word(scan(r, 4), 0);
}


# EN: Admit a nonnegative count bounded by unread bytes before allocating.
# RU: Допустить неотрицательный счётчик в пределах непрочитанных байтов до выделения памяти.
gcount(r: ref GraphReader): int
{
    n := gword(r);
    if (n < 0 || n > len r.bytes-r.at) {
        raise "invalid named graph count";
    }
    return n;
}


# EN: Decode one bounded scalar or link; metadata keeps its original position on the wire.
# RU: Декодировать ограниченный скаляр или ссылку. Метаданные занимают исходное место.
gvalue(r: ref GraphReader, keep: int): ref Value
{
    tag := int scan(r, 1)[0];
    value: ref Value;
    case tag {
    Gundefined =>
        value = ref Value.Undefinedval(0);
    Gnull =>
        value = ref Value.Nullval(0);
    Gbool =>
        n := int scan(r, 1)[0];
        if (n > 1) {
            raise "invalid graph Boolean";
        }
        value = ref Value.Logical(n);
    Gnumber =>
        bytes := scan(r, Wordbytes);
        bits := big 0;
        for (i := 0; i < len bytes; ++i) {
            bits = bits<<8 | big bytes[i];
        }
        value = ref Value.Numeric(math->bits64real(bits));
    Gtext =>
        bytes := scan(r, gcount(r));
        if (keep) {
            copy := array[len bytes] of byte;
            if (len bytes) {
                copy[0:] = bytes;
            }
            bytes = copy;
        }
        value = ref Value.Textual(bytes);
    Glink =>
        value = ref Value.Link(gword(r));
    * =>
        raise "invalid graph value tag";
    }
    gword(r);
    return value;
}


# EN: Decode a counted UTF-8 key exactly. Complex WTF-8 names can be forwarded in the envelope.
# RU: Точно декодировать ключ UTF-8 с длиной. Имена WTF-8 можно переслать в исходной оболочке.
fieldname(r: ref GraphReader): string
{
    bytes := scan(r, gcount(r));
    name := string bytes;
    back := array of byte name;
    if (len back != len bytes) {
        raise "named graph data profile";
    }
    for (i := 0; i < len bytes; ++i) {
        if (bytes[i] != back[i]) {
            raise "named graph data profile";
        }
    }
    return name;
}


# EN: Consume a list of named metadata records. No callback or watcher is invoked.
# RU: Пропустить список именованных записей метаданных. Колбэки и наблюдатели не вызываются.
skipnamed(r: ref GraphReader, kind: int)
{
    count := gcount(r);
    for (i := 0; i < count; ++i) {
        scan(r, gcount(r));
        case kind {
        Callbacks =>
            skipcallback(r);
        Accessors =>
            skipcallback(r);
            skipcallback(r);
        Attributes =>
            gword(r);
        Storedvalues =>
            gvalue(r, 0);
        }
    }
}


# EN: Consume one callback record without resolving its id or symbol.
# RU: Пропустить запись колбэка без разрешения идентификатора или имени.
skipcallback(r: ref GraphReader)
{
    gword(r);
    scan(r, gcount(r));
    scan(r, 5);
}


# EN: Read one record. Preserve only own data for the three supported kinds.
# RU: Прочитать запись. Сохранить только собственные данные трёх поддержанных видов.
gnode(r: ref GraphReader): ref Node
{
    kind := gword(r);
    if (kind == Gtextfield) {
        raise "named graph data profile";
    }
    state := gvalue(r, 0);
    scan(r, 8);
    if (r.version == Ginterfaces) {
        mode := int scan(r, 1)[0];
        if (mode > 1) {
            raise "invalid graph interface mode";
        }
    }
    scan(r, gcount(r)*4);
    node: ref Node;
    if (kind == Tablekind || kind == Arraykind || kind == Objectkind) {
        length := big 0;
        if (kind == Arraykind) {
            pick size := state {
            Numeric =>
                if (!(size.value >= 0.0 && size.value <= real Uintword) ||
                    real (big size.value) != size.value) {
                    raise "named graph data profile";
                }
                length = big size.value;
            * =>
                raise "named graph data profile";
            }
        }
        node = ref Node(kind, length, nil, nil);
    }
    count := gcount(r);
    if (node != nil) {
        node.fields = array[count] of Field;
    }
    for (i := 0; i < count; ++i) {
        if (node != nil) {
            name := fieldname(r);
            node.fields[i] = Field(name, gvalue(r, 1));
        } else {
            scan(r, gcount(r));
            gvalue(r, 0);
        }
    }
    count = gcount(r);
    if (node != nil) {
        node.slots = array[count] of Slot;
    }
    for (i = 0; i < count; ++i) {
        key := big gword(r) & Uintword;
        item := gvalue(r, node != nil);
        if (node != nil) {
            node.slots[i] = Slot(key, item);
        }
    }
    skipnamed(r, Callbacks);
    skipnamed(r, Callbacks);
    skipnamed(r, Accessors);
    skipnamed(r, Attributes);
    skipcallback(r);
    skipcallback(r);
    skipnamed(r, Storedvalues);
    skipnamed(r, Storedvalues);
    return node;
}


# EN/RU: See styxvalue.m. / См. styxvalue.m.
inspect(bytes: array of byte): ref DataGraph
{
    if (!checkgraph(bytes)) {
        raise "invalid named graph envelope";
    }
    version := int bytes[Graphversion];
    if (version != Gbase && version != Ginterfaces) {
        return nil;
    }
    {
        r := ref GraphReader(bytes, Graphheader, version);
        root := gvalue(r, 1);
        count := gcount(r);
        if (count > Gobjects) {
            raise "too many named graph objects";
        }
        nodes := array[count] of ref Node;
        for (i := 0; i < count; ++i) {
            nodes[i] = gnode(r);
        }
        if (r.at != len bytes) {
            raise "invalid named graph trailing bytes";
        }
        graph := ref DataGraph(root, nodes);
        reachable(graph);
        return graph;
    } exception {
    "named graph data profile" =>
        return nil;
    }
    return nil;
}
