// Copyright (c) 2026 bhgv
// EN: Write two known bytes from one worker-owned handle.
// RU: Записать два известных байта через дескриптор одной задачи.


// EN: Write first and second to path and return the acknowledged length on success.
// RU: Записать first и second в path и вернуть подтверждённую длину при успехе.
asinc function writeRemoteFile(path, first, second) {
    var file = vfs.open(path, "w");
    var empty = "";
    var data = empty.split("");
    var size = data.push(first, second);
    var written = file.write(data);
    var closed = file.close();
    return written;
}
