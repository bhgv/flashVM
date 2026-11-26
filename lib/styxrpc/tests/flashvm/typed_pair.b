# Copyright (c) 2026 bhgv
# EN: Exercise the typed Limbo API and portable graphs on an ordinary named channel.
# RU: Проверить типизированный Limbo API и переносимые графы на обычном именованном канале.
implement Client;
include "sys.m";
include "math.m";
include "styxnamed.m";

Client: module { init: fn(); };
sys: Sys; # Runtime namespace for loading the movie and reporting results.
math: Math; # Exact binary64 test.
named: Styxnamed; # Transport and cell leases.
v: Styxvalue; # Typed value codec.
Message, Port, Cell: import named;
Value, Node, Field, Slot, DataGraph: import v;

include "named_movie_upload.b";


# EN: Receive a value message and unwrap its typed ADT.
# RU: Принять сообщение по значению и вернуть его типизированный ADT.
readvalue(port: ref Port): ref Value
{
    pick m := port.recvvalue() {
    Data =>
        return m.value;
    }
    raise "expected named data";
}


# EN: Receive a cell that is already attached and survives subsequent rendezvous.
# RU: Принять уже привязанную ячейку, переживающую следующую встречу.
readcell(port: ref Port): ref Cell
{
    pick m := port.recvvalue() {
    Shared =>
        return m.cell;
    }
    raise "expected a shared named cell";
}


# EN: Compare an exact binary64 value to the expected number.
# RU: Сравнить точное значение binary64 с ожидаемым числом.
checknumber(value: ref Value, expected: real)
{
    pick n := value {
    Numeric =>
        if (math->realbits64(n.value) == math->realbits64(expected)) {
            return;
        }
    }
    raise "wrong typed named number";
}


# EN: Compare counted bytes, including embedded NUL.
# RU: Сравнить байты с длиной, включая встроенный ноль.
checktext(value: ref Value, bytes: array of byte)
{
    pick text := value {
    Textual =>
        if (len bytes == len text.bytes) {
            for (i := 0; i < len bytes; ++i) {
                if (text.bytes[i] != bytes[i]) {
                    raise "wrong typed named text";
                }
            }
            return;
        }
    }
    raise "wrong typed named text length";
}


# EN: Decode one checked snapshot into its portable structured view.
# RU: Декодировать проверенный снимок в переносимое структурное представление.
readgraph(port: ref Port): ref DataGraph
{
    pick value := readvalue(port) {
    Graph =>
        graph := v->inspect(value.bytes);
        if (graph != nil) {
            return graph;
        }
    }
    raise "expected supported named graph";
}


# EN: Resolve one object id in a decoded graph, rejecting nonlinks.
# RU: Разрешить идентификатор объекта в графе, отклоняя значения без ссылки.
node(graph: ref DataGraph, value: ref Value): ref Node
{
    pick link := value {
    Link =>
        return graph.nodes[link.index];
    }
    raise "expected named graph link";
}


# EN: Read an own field by name independently of the producer's field order.
# RU: Прочитать собственное поле по имени независимо от порядка записи.
field(record: ref Node, name: string): ref Value
{
    for (i := 0; i < len record.fields; ++i) {
        if (record.fields[i].name == name) {
            return record.fields[i].value;
        }
    }
    raise "missing named graph field";
}


# EN: Verify scalar framing, dynamic type replacement, automatic attachment and write-through.
# RU: Проверить скаляры, смену типа, автоматическое подключение и запись по ссылке.
values(port: ref Port)
{
    port.sendvalue(ref Message.Data(ref Value.Numeric(-13.25)));
    checknumber(readvalue(port), -26.5);
    payload := array[] of {byte 'A', byte 0, byte 'B'};
    port.sendvalue(ref Message.Data(ref Value.Textual(payload)));
    checktext(readvalue(port), payload);
    pick flag := readvalue(port) {
    Logical =>
        if (!flag.value) {
            raise "wrong typed named Boolean";
        }
    * =>
        raise "expected Boolean";
    }
    original := named->sharevalue(ref Value.Nullval(0));
    pick n := original.getvalue() {
    Nullval =>
        ;
    * =>
        raise "typed cell lost its initial null";
    }
    port.sendvalue(ref Message.Shared(original));
    alias := readcell(port);
    checktext(alias.getvalue(), array of byte "changed");
    alias.setvalue(ref Value.Undefinedval(0));
    pick value := original.getvalue() {
    Undefinedval =>
        ;
    * =>
        raise "typed cell replacement did not reach first lease";
    }
    own := readcell(port);
    checknumber(own.getvalue(), 41.0);
    own.setvalue(ref Value.Numeric(42.25));
    port.sendvalue(ref Message.Data(ref Value.Logical(1)));
    checknumber(readvalue(port), 42.25);
    own.close();
    alias.close();
    original.close();
}


# EN: Send a real FVMG table with a self-cycle and two links to one sparse Array.
# RU: Отправить таблицу FVMG с циклом и двумя ссылками на один разреженный Array.
graphs(port: ref Port)
{
    root := ref Value.Link(0);
    child := ref Value.Link(1);
    members := array[] of {Field("self", root), Field("right", child),
        Field("1", v->text("string-key")), Field("left", child)};
    slots := array[] of {Slot(big 1, ref Value.Numeric(21.0))};
    elements := array[] of {Slot(big 0, ref Value.Numeric(4.0))};
    graph := ref DataGraph(root, array[] of {ref Node(Styxvalue->Tablekind, big 0, members, slots),
        ref Node(Styxvalue->Arraykind, big 3, nil, elements)});
    port.sendvalue(ref Message.Data(v->build(graph)));
    checknumber(readvalue(port), 23.0);
    checknumber(readvalue(port), 77.0);
    back := readgraph(port);
    record := node(back, back.root);
    if (record.kind != Styxvalue->Tablekind || len record.fields != 4 || len record.slots != 1 ||
        record.slots[0].key != big 1 || node(back, field(record, "self")) != record) {
        raise "Limbo-built graph lost its table keys or cycle";
    }
    left := node(back, field(record, "left"));
    right := node(back, field(record, "right"));
    if (left != right || left.kind != Styxvalue->Arraykind || left.length != big 3 ||
        len left.slots != 1 || left.slots[0].key != big 0) {
        raise "Limbo-built graph lost its shared sparse Array";
    }
    checktext(field(record, "1"), array of byte "named");
    checknumber(record.slots[0].value, 77.0);
    checknumber(left.slots[0].value, 20.0);

    flashgraph := readgraph(port);
    rootArray := node(flashgraph, flashgraph.root);
    if (rootArray.kind != Styxvalue->Arraykind || rootArray.length != big 2 ||
        len rootArray.slots != 2 || len rootArray.fields != 1 ||
        rootArray.fields[0].name != "self" || node(flashgraph, rootArray.fields[0].value) != rootArray) {
        raise "cannot inspect FlashVM's ordinary Array graph";
    }
    checktext(rootArray.slots[0].value, array of byte "first");
    checktext(rootArray.slots[1].value, array of byte "second");
}


# EN: Reject corruption while preserving signed zero and both absent-value tags.
# RU: Отклонить повреждение, сохранить знаковый ноль и оба тега отсутствующего значения.
localcodec()
{
    sign := math->bits64real(big 16r8000000000000000);
    checknumber(v->decode(v->encode(ref Value.Numeric(sign))), sign);
    frame := v->encode(v->build(ref DataGraph(ref Value.Nullval(0), nil)));
    # Independent CRC-32 of the null-root record with no object nodes.
    # Независимая CRC-32 записи с корнем null и без узлов.
    if (int frame[13] != 16r6b || int frame[14] != 16raf ||
        int frame[15] != 16r20 || int frame[16] != 16r7b) {
        raise "incorrect IEEE CRC-32";
    }
    frame[len frame-1] ^= byte 1;
    {
        v->decode(frame);
    } exception {
    "invalid named value frame" =>
        return;
    }
    raise "bad graph checksum was accepted";
}


# EN: Run the exchange using only the paired namespace and project-built assets.
# RU: Выполнить обмен в парном пространстве имён с собранными средствами проекта файлами.
init()
{
    sys = load Sys Sys->PATH;
    math = load Math Math->PATH;
    named = load Styxnamed Styxnamed->PATH;
    v = load Styxvalue Styxvalue->PATH;
    if (named == nil || v == nil || math == nil) {
        raise "missing typed named modules";
    }
    v->init();
    named->init();
    localcodec();
    port := named->open("/typed");
    start("typed.swf");
    values(port);
    graphs(port);
    port.close();
    sys->print("PASS native typed RPC paired VMs\n");
}
