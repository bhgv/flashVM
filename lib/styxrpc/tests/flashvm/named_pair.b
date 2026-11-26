# Copyright (c) 2026 bhgv
# EN: Drive a stock named-channel rendezvous from the paired Limbo application.
# RU: Проверить штатный именованный канал из парного приложения Limbo.
implement Client;
include "sys.m";
include "styxnamed.m";

Client: module { init: fn(); };
sys: Sys; # Paired runtime filesystem and scheduler.
named: Styxnamed; # Limbo binding to the common native registry.
Port, Cell: import named;


include "named_movie_upload.b";


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
    start("named.swf");
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
    cell := refexchange(port, graph);
    graphexchange(port, graph);
    indexexchange(port);
    raw := nativeindices(port);
    snapshotexchange(port, raw, graph);
    port.close();
    for (i := 0; i < 100; ++i) {
        (exists, meta) = sys->stat("/jobs");
        if (exists < 0) {
            if (!equal(cell.get(), count111())) {
                raise "closing the channel retired a live cell lease";
            }
            value := raw.get();
            if (len value != 7 || int value[0] != Styxnamed->Text || string value[1:] != "edited") {
                raise "closing the channel retired a live numeric cell lease";
            }
            cell.close();
            raw.close();
            sys->print("PASS native named RPC paired VMs\n");
            return;
        }
        sys->sleep(10);
    }
    raise "last close did not release the name";
}


# EN: Construct the binary64 frame for 111 used in the alias write proof.
# RU: Собрать кадр binary64 числа 111 для проверки записи через ссылку.
count111(): array of byte
{
    return array[] of {byte Styxnamed->Number, byte 16r40, byte 16r5b, byte 16rc0,
        byte 0, byte 0, byte 0, byte 0, byte 0};
}


# EN: Read, change type, forward, and observe changes to one cell in both VMs.
# RU: Прочитать ячейку, сменить тип, передать её обратно и проверить изменения в обеих VM.
refexchange(port: ref Port, snapshot: array of byte): ref Cell
{
    descriptor := port.recv();
    cell := named->adopt(descriptor);
    before := array[] of {byte Styxnamed->Number, byte 16r40, byte 16r35, byte 0,
        byte 0, byte 0, byte 0, byte 0, byte 0};
    if (!equal(cell.get(), before)) {
        raise "the new reference did not expose the AS source";
    }
    changed := array[] of {byte Styxnamed->Text, byte 'B', byte 0, byte 'C'};
    cell.set(changed);
    flag := array[] of {byte Styxnamed->Boolean, byte 1};
    port.send(flag);
    if (!equal(port.recv(), changed)) {
        raise "AS value did not observe the changed scalar";
    }
    if (!equal(port.recv(), descriptor) || !equal(cell.get(), flag)) {
        raise "AS assignment or forwarded cell identity changed";
    }
    port.send(cell.payload());
    after := count111();
    if (!equal(port.recv(), after) || !equal(cell.get(), after)) {
        raise "assignment through the returned AS alias did not reach the source";
    }
    own := named->share(snapshot);
    port.send(own.payload());
    if (!equal(port.recv(), snapshot)) {
        raise "AS did not read the graph cell created by Limbo";
    }
    result := port.recv();
    if (len result != 7 || int result[0] != Styxnamed->Text || string result[1:] != "edited" ||
        !equal(own.get(), result)) {
        raise "AS alias did not update the cell created by Limbo";
    }
    own.close();
    return cell;
}


# EN: Replace a live graph with an array, then restore it through the returned alias.
# RU: Заменить живой граф массивом, затем восстановить граф через полученную ссылку.
graphexchange(port: ref Port, replacement: array of byte)
{
    original := port.recv();
    if (len original <= 16384 || int original[0] != Styxnamed->Snapshot) {
        raise "the AS object did not produce a multi-fragment snapshot";
    }
    graph := named->adopt(port.recv());
    if (!equal(graph.get(), original)) {
        raise "the graph reference lost the original snapshot";
    }
    port.send(graph.payload());
    ready := array[] of {byte Styxnamed->Boolean, byte 1};
    if (!equal(port.recv(), original)) {
        raise "the graph snapshot lost shared array edges or a self-cycle";
    }
    graph.set(replacement);
    port.send(ready);
    if (!equal(port.recv(), replacement)) {
        raise "the original AS slot did not see the array replacement";
    }
    if (!equal(port.recv(), original) || !equal(graph.get(), original)) {
        raise "assigning through the AS alias did not restore the graph";
    }
    if (!equal(port.recv(), original)) {
        raise "an edit to a local snapshot reached the shared cell without assignment";
    }
    edited := port.recv();
    if (equal(edited, original) || !equal(graph.get(), edited)) {
        raise "an assigned snapshot edit did not reach the shared cell";
    }
    graph.close();
    field := named->adopt(port.recv());
    field.set(count111());
    port.send(field.payload());
    if (!equal(port.recv(), count111()) || !equal(port.recv(), field.payload()) ||
        !equal(port.recv(), count111())) {
        raise "capturing ref model.count again lost the updated field";
    }
    field.close();
}


# EN: Both runtimes read and write the same indexed slot. A noncanonical key stays separate.
# RU: Обе VM читают и записывают один элемент. Неканоническое имя остаётся отдельным.
indexexchange(port: ref Port)
{
    one := array[] of {byte Styxnamed->Number, byte 16r3f, byte 16rf0, byte 0,
        byte 0, byte 0, byte 0, byte 0, byte 0};
    port.send(one);
    numeric := port.recv();
    slot := named->adopt(numeric);
    initial := array[] of {byte Styxnamed->Number, byte 16r40, byte 16r35, byte 0,
        byte 0, byte 0, byte 0, byte 0, byte 0};
    if (!equal(slot.get(), initial)) {
        raise "capturing the AS array index did not expose the current element";
    }
    slot.set(count111());
    port.send(slot.payload());
    if (!equal(port.recv(), count111())) {
        raise "reading the AS array index bypassed the shared cell";
    }
    local := array[] of {byte Styxnamed->Number, byte 16r40, byte 16r5e, byte 16r40,
        byte 0, byte 0, byte 0, byte 0, byte 0};
    if (!equal(port.recv(), local) || !equal(slot.get(), local)) {
        raise "writing the AS array index bypassed the shared cell";
    }
    if (!equal(port.recv(), numeric)) {
        raise "numeric and string subscripts did not capture the same array index";
    }
    namedframe := port.recv();
    field := named->adopt(namedframe);
    if (equal(namedframe, numeric) || !equal(field.get(), initial)) {
        raise "the noncanonical string key collapsed onto the numeric index";
    }
    field.set(count111());
    port.send(one);
    if (!equal(port.recv(), count111()) || !equal(port.recv(), local)) {
        raise "AS reads did not distinguish the computed key and index";
    }
    field.close();
    slot.close();
}


# EN: Share one raw numeric slot, assign a received alias at UInt32 max and verify ownership.
# RU: Разделить числовой слот, принять ссылку по максимальному UInt32 и проверить владение.
nativeindices(port: ref Port): ref Cell
{
    descriptor := port.recv();
    cell := named->adopt(descriptor);
    initial := array[] of {byte Styxnamed->Number, byte 16r40, byte 16r35, byte 0,
        byte 0, byte 0, byte 0, byte 0, byte 0};
    if (!equal(cell.get(), initial)) {
        raise "native table publication selected the wrong namespace";
    }
    cell.set(count111());
    port.send(cell.payload());
    if (!equal(port.recv(), count111())) {
        raise "native numeric read did not observe the peer's write";
    }
    value := port.recv();
    if (len value != 7 || int value[0] != Styxnamed->Text || string value[1:] != "edited" ||
        !equal(cell.get(), value)) {
        raise "assigning through the native numeric alias did not update the cell";
    }
    field := port.recv();
    if (len field != 6 || int field[0] != Styxnamed->Text || string field[1:] != "named") {
        raise "the numeric slot changed the string with the same spelling";
    }
    if (!equal(port.recv(), descriptor) || !equal(port.recv(), descriptor)) {
        raise "forwarding the native slot or its numeric alias lost identity";
    }
    namedkey := port.recv();
    word := named->adopt(namedkey);
    if (equal(namedkey, descriptor) || !equal(word.get(), field)) {
        raise "the named key and numeric index published the same cell";
    }
    word.set(count111());
    port.send(count111());
    if (!equal(port.recv(), count111()) || !equal(port.recv(), value)) {
        raise "named cell write also affected the numeric cell";
    }
    word.close();
    return cell;
}


# EN: Sample a cyclic graph with an AS alias and two leases to the same value cell.
# RU: Снять циклический граф с привязкой AS и двумя арендами одной ячейки.
snapshotexchange(port: ref Port, source: ref Cell, graph: array of byte)
{
    before := source.get();
    source.set(graph);
    port.send(count111());
    snapshot := port.recv();
    if (len snapshot < 2 || int snapshot[0] != Styxnamed->Snapshot) {
        raise "a table containing live bindings did not produce a detached graph";
    }
    port.send(snapshot);
    for (i := 0; i < 4; ++i) {
        if (!equal(port.recv(), count111())) {
            raise "the sampled graph lost its cycle, shared identities, or distinct key";
        }
    }
    number := array[] of {byte Styxnamed->Number, byte 16r40, byte 16r35, byte 0,
        byte 0, byte 0, byte 0, byte 0, byte 0};
    if (!equal(port.recv(), number) || !equal(port.recv(), graph) || !equal(source.get(), graph)) {
        raise "editing the received snapshot changed the source cell";
    }
    captured := named->adopt(port.recv());
    if (!equal(captured.get(), snapshot)) {
        raise "creating a new cell did not capture the live slots of the assigned graph";
    }
    captured.close();
    source.set(before);
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
