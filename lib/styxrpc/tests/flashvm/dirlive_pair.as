// Copyright (c) 2026 bhgv
// EN: Mutate one export while continuing to access a held DisVM directory.
// RU: Менять один экспорт, продолжая обращаться к открытому каталогу DisVM.


// EN: Open the DisVM directory once and rescan its exact fid after peer publication and withdrawal.
// RU: Один раз открыть каталог DisVM и пересканировать тот же fid после публикации и отзыва.
function onEnterFrame() {
    if (!directoryReady) {
        directoryPhase = 0;
        keptDir = os.opendir("/tools");
        var item = keptDir.next_file();
        var before = item.name + "|";
        var eof = keptDir.next_file();
        var i = 0;
        var padding = "-abcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnopabcdefghijklmnop";
        while (80 - i) {
            var placed = vfs.newnode("/held/" + i + padding, "x", changeDirectoryPhase);
            i = i + 1;
        }
        var outside = vfs.newnode("/other/anchor", "x", changeDirectoryPhase);
        var gone = vfs.newnode("/vanish/old", "x", changeDirectoryPhase);
        if (!before.indexOf("before|")) {
            if (!eof) {
                var ready = vfs.newnode("/dirSignal", "x", changeDirectoryPhase);
            }
        }
        directoryReady = true;
    }
    if (directoryPhase) {
        var reset = keptDir.rewind();
        var entry = keptDir.next_file();
        var sawBefore = false;
        var sawReread = false;
        while (entry) {
            var name = entry.name + "|";
            if (!name.indexOf("before|")) {
                sawBefore = true;
            }
            if (!name.indexOf("reread|")) {
                sawReread = true;
            }
            entry = keptDir.next_file();
        }
        if (sawBefore) {
            if (!(directoryPhase - 1)) {
                if (sawReread) {
                    var unrelated = vfs.newnode("/other/added", "x", changeDirectoryPhase);
                }
            } else {
                if (!sawReread) {
                    var removed = vfs.rmnode("/vanish/old");
                    var reopened = vfs.newnode("/vanish/new", "x", changeDirectoryPhase);
                    var changed = vfs.newnode("/held/new", "x", changeDirectoryPhase);
                }
                var closed = keptDir.close();
            }
        }
        directoryPhase = 0;
    }
}
