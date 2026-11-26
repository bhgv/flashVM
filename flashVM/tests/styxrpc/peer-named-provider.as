// Copyright (c) 2026 bhgv
// EN: Match the other movie's named rendezvous and check its live cell writes.
// RU: Выполнить обмен с другим фильмом и проверить запись в живые ячейки.


// EN: Read and edit the snapshot, forward an alias, lend a slot and wait for disconnect.
// RU: Прочитать и изменить снимок, переслать ссылку, передать слот и ждать разрыва.
function onEnterFrame() {
    if (!namedDone) {
        var port = os.chan("paired-work");
        var copy = port.recv();
        var left = copy.left;
        var right = copy.right;
        left.mark = 77;
        var self = copy.self;
        self.tag = 12;
        var bytes = copy.data;
        var alias = port.recv();
        var arrived = alias;
        alias = "edited";
        var forwarded = port.send(ref alias);
        var counter = port.recv();
        var observed = alias;

        var other = 15;
        var refSent = port.send(ref other);
        var received = port.recv();
        var released = port.delete();
        var finish = vfs.chan("/paired-finish");
        var proof = bytes.length + "," + right.mark + "," + copy.tag + "," + arrived + "," + counter + "," + observed + "," + received + "," + other;
        var reported = finish.send(proof);
        var pending = finish.recv();
        namedDone = true;
    }
}
