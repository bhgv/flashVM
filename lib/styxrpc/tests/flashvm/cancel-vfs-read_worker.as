// Copyright (c) 2026 bhgv
// EN: Read a byte prefix on one task-owned remote fid.
// RU: Прочитать префикс байтов на удалённом fid одной задачи.


// EN: Open path in this worker, read count bytes and close on normal completion.
// RU: Открыть path в этой задаче, прочитать count байтов и закрыть при обычном завершении.
asinc function readRemoteFile(path, count) {
    var file = vfs.open(path);
    var bytes = file.read(count);
    var closed = file.close();
    return bytes[0];
}
