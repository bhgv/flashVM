// Copyright (c) 2026 bhgv
// EN: Rendezvous with Limbo on the already attached named channel.
// RU: Обменяться с Limbo в именованном канале уже подключённой сессии.


// EN: Exercise value and reference transfer over the same attached session.
// RU: Проверить передачу значений и ссылок в одной подключённой сессии.
function onEnterFrame() {
    if (!done) {
        var entered = os.cd("/named-area");
        var port = vfs.chan("../jobs");
        var input = port.recv();
        var sent = port.send(input);
        var numbers = "3,7";
        var values = numbers.split(",");
        var graphSent = port.send(values);
        var copy = port.recv();
        var graphEcho = port.send(copy);
        var counter = 21;
        var exported = port.send(ref counter);
        var changed = port.recv();
        var snapshot = port.send(counter);
        counter = true;
        var forwarded = port.send(ref counter);
        var alias = port.recv();
        alias = 111;
        var confirmed = port.send(counter);
        var foreign = port.recv();
        var foreignGraph = port.send(foreign);
        foreign = "edited";
        var returned = port.send(foreign);

        // The array carries a cycle and two edges to the same nested array.
        // Массив содержит цикл и две ссылки на один вложенный массив.
        var labels = "one,two";
        var parts = labels.split(",");
        var graph = labels.split(",");
        graph.left = parts;
        graph.right = parts;
        graph.self = graph;
        var data = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
        var doubles = 8;
        while (doubles) {
            data = data + data;
            doubles = doubles - 1;
        }
        graph.data = data;
        var original = port.send(graph);
        var published = port.send(ref graph);
        var graphAlias = port.recv();
        var current = graphAlias;
        var links = port.send(current);
        var update = port.recv();
        var observed = port.send(graph);
        graphAlias = current;
        var restored = port.send(graph);
        current.revision = 2;
        var unchanged = port.send(graph);
        graphAlias = current;
        var editedGraph = port.send(graph);
        var model = labels.split(",");
        model.count = 21;
        var field = port.send(ref model.count);
        var modelCount = port.recv();
        var written = port.send(modelCount);
        var recaptured = port.send(ref model.count);
        var namedRead = port.send(model.count);

        // Capture one canonical index, then address the same slot by number and by string.
        // Захватить один индекс и обратиться к нему числом и строкой.
        var index = 1;
        model[index] = 21;
        var indexed = port.send(ref model[port.recv()]);
        var entry = port.recv();
        var readIndex = port.send(model[index]);
        model[index] = 121;
        var readAlias = port.send(entry);
        var sameSlot = port.send(ref model["1"]);

        // A computed noncanonical key must remain a distinct named member.
        // Вычисляемый неканонический ключ остаётся отдельным именованным полем.
        var key = "01";
        model[key] = 21;
        var keyed = port.send(ref model[key]);
        var access = port.recv();
        var readKey = port.send(model[key]);
        var stillIndex = port.send(model[index]);
        var retired = port.delete();
        done = true;
    }
}
