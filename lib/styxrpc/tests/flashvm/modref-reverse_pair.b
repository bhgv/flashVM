# Copyright (c) 2026 bhgv
# EN: Hold a real Limbo Int32 behind a public module variable and check both VMs' aliases.
# RU: Удержать Int32 Limbo за открытой переменной модуля и проверить ссылки из обеих VM.
implement Client;
include "sys.m";
include "styxnamed.m";
include "styxmodref.m";
include "styxnodes.m";

Client: module { init: fn(); };
sys: Sys; # Runtime file and process operations.
named: Styxnamed; # The paired named channel.
refs: Styxmodref; # Exact Int32 reference handles.
nodes: Styxnodes; # Guest-owned positioned export.
Cell: import refs;
Port: import named;
Node: import nodes;
Export: con "/mounted/counter.dis/.var/value"; # Reused after withdrawing the original.

include "named_movie_upload.b";


# EN: Decode a four-byte signed value using the same representation as the mounted file.
# RU: Прочитать знаковое четырёхбайтовое значение в представлении файла.
decode(bytes: array of byte): int
{
    value := int bytes[0];
    if (value >= 128) {
        value -= 256;
    }
    for (i := 1; i < 4; ++i) {
        value = value * 256 + int bytes[i];
    }
    return value;
}


# EN: Serve a finite number of exact accesses in a dedicated Limbo thread and report its value.
# RU: Обслужить заданное число точных обращений в отдельном потоке Limbo и вернуть его значение.
serve(node: ref Node, value, requests: int, done: chan of int)
{
    for (request := 0; request < requests; ++request) {
        (item, at) := nodes->recvat(node);
        if (at != big 0 || len item.buffer != 4) {
            node.refuse(item);
            raise "unexpected module variable transfer extent";
        }
        if (item.operation == "r") {
            item.buffer[0] = byte (value >> 24);
            item.buffer[1] = byte (value >> 16);
            item.buffer[2] = byte (value >> 8);
            item.buffer[3] = byte value;
        } else if (item.operation == "w") {
            value = decode(item.buffer);
        } else {
            node.refuse(item);
            raise "unexpected module variable operation";
        }
        if (!node.reply(item, 4)) {
            raise "module variable response was not accepted";
        }
    }
    done <-= value;
}


# EN: Exercise lazy open and return whether this alias can still read.
# RU: Проверить отложенное открытие и вернуть, если чтение ещё возможно.
readable(cell: ref Cell): int
{
    {
        value := cell.read();
    } exception e {
    "*" =>
        return 0;
    }
    return 1;
}


# EN: Test write permission without changing a correct read-only alias.
# RU: Проверить право записи без изменения ссылки, открытой только для чтения.
writable(cell: ref Cell): int
{
    {
        cell.write(17);
    } exception e {
    "*" =>
        return 0;
    }
    return 1;
}


# EN: Compare counted payloads without using their string representation.
# RU: Сравнить кадры с длиной без преобразования в строки.
same(a, b: array of byte): int
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


# EN: Check one complete text reply from FlashVM's named channel.
# RU: Проверить один полный текстовый ответ в именованном канале FlashVM.
expect(pipe: ref Port, expected: string)
{
    proof := pipe.recv();
    if (int proof[0] != Styxnamed->Text || string proof[1:] != expected) {
        raise sys->sprint("FlashVM module reference proof differs: %s", string proof[1:]);
    }
}


# EN: Wait for the FD close worker to remove the exact callback publication.
# RU: Дождаться снятия точной публикации потоком закрытия FD.
wait_retired()
{
    for (attempt := 0; attempt < 200; ++attempt) {
        (available, nil) := sys->stat(Export);
        if (available < 0) {
            return;
        }
        sys->sleep(10);
    }
    raise "closed Limbo publication is still present";
}


# EN: Create a replacement only after the original owner and both wire aliases have been exercised.
# RU: Создать замену после проверки исходного владельца и обеих переданных ссылок.
init()
{
    sys = load Sys Sys->PATH;
    named = load Styxnamed Styxnamed->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    refs = load Styxmodref Styxmodref->PATH;
    if (named == nil || nodes == nil || refs == nil) {
        raise "cannot load module variable support";
    }
    named->init();
    nodes->init();
    refs->init();
    pipe := named->open("/dis-module-refs");
    replacement := named->open("/dis-module-replacement");
    node := nodes->publishat(Export, "rw", big 4);
    done := chan of int;
    spawn serve(node, 41, 8, done);
    start("modref-reverse.swf");
    wire := pipe.recv();
    observe := pipe.recv();
    cell := refs->adopt(wire);
    observer := refs->adopt(observe);
    unopened := refs->adopt(observe);
    wrongpath := array[len wire] of byte;
    wrongpath[0:] = wire;
    wrongpath[len wire-1] = byte 'x';
    wrong := refs->adopt(wrongpath);
    if (readable(wrong) || cell.read() != 41 || writable(observer)) {
        raise "initial value, exact pathname or read-only permissions differ";
    }
    wrong.close();
    maximum := 2147483647;
    minimum := -2147483647-1;
    cell.write(maximum);
    if (observer.read() != maximum || !same(cell.payload(), wire) || !same(observer.payload(), observe)) {
        raise "write-through or original reference frame differs";
    }
    pipe.send(cell.payload());
    pipe.send(observer.payload());
    expect(pipe, "41,2147483647,-2147483648");
    pipe.close();
    if (cell.read() != minimum || <-done != minimum) {
        raise "assignment in FlashVM missed the original Limbo value";
    }
    node.close();
    wait_retired();
    fresh := nodes->publishat(Export, "r", big 4);
    spawn serve(fresh, 77, 2, done);
    if (readable(cell) || readable(observer) || readable(unopened)) {
        raise "the replacement admitted a stale module reference";
    }
    ready := sys->create("/module-ref-replacement", Sys->OWRITE, 8r666);
    if (ready == nil) {
        raise sys->sprint("cannot signal replacement: %r");
    }
    ready = nil;
    expect(replacement, "value=77");
    frame := replacement.recv();
    alias := refs->adopt(frame);
    if (same(observe[5:13], frame[5:13]) || alias.read() != 77 || writable(alias) || <-done != 77) {
        raise "replacement Qid, value or permissions differ";
    }
    replacement.close();
    alias.close();
    fresh.close();
    unopened.close();
    observer.close();
    cell.close();
    sys->print("PASS native modref-reverse RPC paired VMs\n");
}
