// Copyright (c) 2026 bhgv
// EN: Lend a worker-local channel to a Dis function and receive from its returned channel.
// RU: Передать локальный канал задачи функции Dis и прочитать из возвращённого канала.


// EN: Send input through a typed queue and return the numeric result from the remote lease.
// RU: Послать input через типизированную очередь и вернуть число из удалённой аренды.
asinc function callScoped(input) {
    var image = vfs.open("/mounted/asinc-scope.dis");
    var proxy = image.load();
    var closed = image.close();
    var queue = vfs.localChannel("i", 1);
    var sent = queue.trySend(input);
    var reply = proxy.bounce(queue);
    var value = reply.recv();
    var retired = reply.close();
    var localClosed = queue.close();
    return value;
}
