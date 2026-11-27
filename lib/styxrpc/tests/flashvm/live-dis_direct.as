// Copyright (c) 2026 bhgv
// EN: Exercise the native reverse scope directly from the playback owner.
// RU: Проверить нативный встречный контекст непосредственно из потока фильма.


// EN: Call managed, graph and channel exports without starting an AS task.
// RU: Вызвать управляемый, графовый и канальный экспорты без отдельной задачи AS.
function directDisProof() {
    var image = vfs.open("/library/target.dis");
    var proxy = image.load();
    var imageClosed = image.close();
    var label = proxy.label("movie") + "|";
    var empty = "";
    var leaf = empty.split("");
    var leafCount = leaf.push(40);
    var box = empty.split("");
    var boxCount = box.push(leaf);
    var incremented = proxy.measure(box);
    var queue = vfs.localChannel("i", 1);
    var sent = queue.trySend(41);
    var reply = proxy.bounce(queue);
    var answer = reply.recv();
    var replyClosed = reply.close();
    var queueClosed = queue.close();
    return label + incremented + "|" + leaf[0] + "|" + answer + "|";
}
