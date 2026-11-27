// Copyright (c) 2026 bhgv
// EN: Use the attached file tree in the worker and return a copied byte.
// RU: Использовать подключённое дерево файлов в потоке и вернуть считанный байт.
asinc function readFromVfs(path) {
    var before = os.pwd();
    var changed = vfs.cd("/tools");
    var here = os.pwd();
    var dir = vfs.opendir(".");
    var entry = dir.next_file();
    var visible = 0;
    var hidden = 0;
    while (entry) {
        var name = entry.name + "|";
        if (!name.indexOf("input|")) {
            visible = 1;
        }
        if (!name.indexOf(".prog|")) {
            hidden = 1;
        }
        entry = dir.next_file();
    }
    var closed = dir.close();
    var attrs = os.attr(path);
    var input = vfs.open(path);
    var bytes = input.read(1);
    var inputClosed = input.close();
    var output = os.open("result", "rw");
    var written = output.write(bytes);
    var flushed = output.flush();
    var outputClosed = output.close();
    var created = vfs.mkdir("/task-temp");
    var removed = os.rm("/task-temp");
    var all = before + "|" + here + "|" + visible + "|" + hidden + "|" + attrs.length + "|" + written + "|";
    var result = 0;
    if (!all.indexOf("/|/tools|1|0|1|1|")) {
        if (flushed) {
            result = bytes[0];
        }
    }
    return result;
}
