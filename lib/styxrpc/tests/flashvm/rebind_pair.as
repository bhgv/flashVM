// Copyright (c) 2026 bhgv
// EN: Read fresh and retained VFS objects while a third DisVM thread changes the namespace.
// RU: Читать новые и удерживаемые объекты VFS, пока третий поток DisVM меняет пространство имён.


// EN: Check one committed namespace state per frame while preserving the original file.
// RU: Проверить одно принятое состояние пространства имён за кадр, удерживая исходный файл.
function onEnterFrame() {
    if (!rebindStarted) {
        rebindPhase = 0;
        var moved = vfs.cd("/live");
        originalFile = os.open("value");
        originalDir = vfs.opendir(".");
        var first = originalFile.read(1);
        var line = first.join(",") + "|";
        if (!line.indexOf("65|")) {
            var ready = vfs.newnode("/rebind-signal", "x", rebindSignal);
        }
        rebindStarted = true;
    }
    if (rebindPhase) {
        var file = vfs.open("value");
        var bytes = file.read(1);
        var stat = os.attr("value");
        var size = file.len();
        var closedFile = file.close();
        var back = originalFile.seek(0);
        var retained = originalFile.read(1);
        var dir = originalDir;
        if (rebindPhase - 1) {
            dir = os.opendir(".");
        }
        var mask = 0;
        var tailExists = false;
        var entry = dir.next_file();
        while (entry) {
            var key = entry.name + "|";
            if (!key.indexOf("base-only|")) {
                mask = mask + 1;
            }
            if (!key.indexOf("tail-only|")) {
                mask = mask + 2;
                tailExists = true;
            }
            if (!key.indexOf("front-only|")) {
                mask = mask + 4;
            }
            if (!key.indexOf(".prog|")) {
                mask = mask + 64;
            }
            entry = dir.next_file();
        }
        var closedDir = dir.close();
        var fallback = 0;
        if (tailExists) {
            var tail = vfs.open("tail-only");
            var data = tail.read(1);
            var tailClosed = tail.close();
            fallback = data[0] - 66;
        }
        var evidence = bytes.join(",") + "|" + retained.join(",") + "|" + mask + "|" + size + "|" + stat.length + "|" + fallback + "|";
        var expectedSize = rebindByte - 64;
        var expected = rebindByte + "|65|" + rebindNames + "|" + expectedSize + "|" + expectedSize + "|0|";
        if (!evidence.indexOf(expected)) {
            var ack = vfs.newnode("/rebind-acks/done" + rebindPhase, "r", rebindSignal);
        }
        rebindPhase = 0;
    }
}
