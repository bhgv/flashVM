# Copyright (c) 2026 bhgv
# EN: Rebind the exported library directory while FlashVM retains its first proxy.
# RU: Перепривязать каталог библиотек, пока FlashVM удерживает первый прокси.
implement Client;
include "sys.m";
include "styxnodes.m";

Client: module { init: fn(); };
sys: Sys; # System calls for this fixture.
nodes: Styxnodes; # Owns the deliberately stalled image file.
Node: import nodes;
CancelBudgetMs: con 3000; # Less than both the SDK poll deadline and the node deadline.

include "named_movie_upload.b";


# EN: Wait for a marker created by the movie through ordinary VFS.
# RU: Дождаться метки, созданной фильмом через обычную VFS.
wait_for(path: string)
{
    for (i := 0; i < 1200; i++) {
        (ok, nil) := sys->stat(path);
        if (ok == 0) {
            return;
        }
        sys->sleep(10);
    }
    raise "missing live module test marker " + path;
}


# EN: Release an AS gate by writing the exact one-byte marker at its origin.
# RU: Открыть барьер AS записью одного байта в начало файла.
signal(path: string)
{
    gate := sys->open(path, Sys->OWRITE);
    ready := array of byte "1";
    if (gate == nil || sys->write(gate, ready, 1) != 1) {
        raise "cannot signal live module test gate " + path;
    }
}


# EN: Cancel only after the resident's native image read has reached the positioned node.
# RU: Отменить задачу только после прихода нативного чтения образа к позиционному узлу.
cancel_pending(image: ref Node, bytes: array of byte)
{
    (read, offset) := nodes->recvat(image);
    if (read == nil || read.operation != "r" || offset != big 0) {
        raise "expected the resident image read";
    }
    started := sys->millisec();
    signal("/cancel-gate");
    wait_for("/cancelled");
    (again, position) := nodes->recvat(image);
    if (sys->millisec()-started > CancelBudgetMs || image.reply(read, 0)) {
        raise "cancelled Dis load still accepts its original image reply";
    }
    if (again == nil || again.operation != "r" || position != big 0 || len again.buffer != len bytes) {
        raise "the next load did not obtain a fresh image read";
    }
    again.buffer[:] = bytes;
    if (!image.reply(again, len bytes)) {
        raise "cannot serve the next image read";
    }
    wait_for("/callback-image");
    image.close();
}


# EN: Replace the live union head after the first proxy has made its calls.
# RU: Заменить первый слой живого union после вызовов через первый прокси.
init()
{
    sys = load Sys Sys->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    if (nodes == nil) {
        raise "cannot load callback file support";
    }
    nodes->init();
    fixture := sys->open("/scalar.dis", Sys->OREAD);
    (ok, attributes) := sys->fstat(fixture);
    if (fixture == nil || ok < 0 || attributes.length <= big 0) {
        raise "cannot inspect the test image";
    }
    bytes := array[int attributes.length] of byte;
    if (sys->readn(fixture, bytes, len bytes) != len bytes) {
        raise "cannot read the test image";
    }
    image := nodes->publishat("/slow.dis", "rx", attributes.length);
    start("live-dis.swf");
    cancel_pending(image, bytes);
    wait_for("/loaded");
    wait_for("/direct");
    if (sys->bind("/replacement", "/library", Sys->MBEFORE) < 0) {
        raise sys->sprint("cannot replace live library directory: %r");
    }
    signal("/gate");
    wait_for("/done");
    wait_for("/closed-method");
    sys->print("PASS native live-dis RPC paired VMs\n");
}
