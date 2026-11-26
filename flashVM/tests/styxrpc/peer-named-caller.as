// Copyright (c) 2026 bhgv
// EN: Transfer a graph and live AS variables through one paired named service.
// RU: Передать граф и живые переменные AS через одну именованную службу пары.


// EN: Send a detached graph, share a source slot, and update the other peer's slot.
// RU: Отправить независимый граф, разделить свой слот и изменить слот партнёра.
function onEnterFrame() {
    if (!namedDone) {
        var port = vfs.chan("/paired-work");
        var seed = "one,two";
        var graph = seed.split(",");
        var leaf = seed.split(",");
        graph.left = leaf;
        graph.right = leaf;
        graph.self = graph;
        var data = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        var doubles = 8;
        while (doubles) {
            data = data + data;
            doubles = doubles - 1;
        }
        graph.data = data;
        var graphSent = port.send(graph);

        var counter = 41;
        var refSent = port.send(ref counter);
        var alias = port.recv();
        var changed = alias;
        alias = 77;
        var sentCounter = port.send(counter);
        var other = port.recv();
        var initialOther = other;
        other = 99;
        var sentOther = port.send(other);
        var released = port.delete();

        var finish = os.chan("paired-finish");
        var proof = finish.recv();
        var value = counter;
        var live = other;
        var reserved = os.stat("/.channels");
        var transcript = "[" + proof + "|" + changed + "," + initialOther + "," + value + "," + live + "," + reserved[0] + "]";
        if (!transcript.indexOf("[16384,77,12,41,77,77,99,99|edited,15,77,99,1]")) {
            var reported = _root.reportPeerResult();
            namedDone = true;
        }
    }
}
