// Copyright (c) 2026 bhgv
// EN: Exchange typed Limbo values and a portable cyclic graph with the stock player.
// RU: Обменяться значениями Limbo и переносимым циклическим графом штатным плеером.


// EN: Receive, edit and return values and live references through one session.
// RU: Принять, изменить и вернуть значения и живые ссылки в одной сессии.
function onEnterFrame() {
    if (!done) {
        var port = vfs.chan("/typed");
        var number = port.recv();
        var doubled = port.send(number * 2);
        var text = port.recv();
        var echoed = port.send(text);
        var enabled = port.send(true);

        var alias = port.recv();
        alias = "changed";
        var forwarded = port.send(ref alias);
        var own = 41;
        var published = port.send(ref own);
        var updated = port.recv();
        var readOwn = port.send(own);

        var graph = port.recv();
        var left = graph.left;
        var right = graph.right;
        var self = graph.self;
        left[0] = 20;
        var sharedArray = port.send(right[0] + right.length);
        graph["1"] = "named";
        graph[1] = 77;
        var selfIndex = port.send(self[1]);
        var graphBack = port.send(graph);

        var textList = "first,second";
        var fromFlash = textList.split(",");
        fromFlash.self = fromFlash;
        var returned = port.send(fromFlash);
        var retired = port.delete();
        done = true;
    }
}
