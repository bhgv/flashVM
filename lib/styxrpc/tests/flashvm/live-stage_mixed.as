// Copyright (c) 2026 bhgv
// EN: Observe copy-back, then keep a result channel live after unloading the issuing proxy.
// RU: Проверить обратную запись и удержать канал результата после освобождения прокси модуля.


// EN: Observe copy-back, then keep a result channel live after unloading the issuing proxy.
// RU: Проверить обратную запись и удержать канал результата после освобождения прокси модуля.
function proveMixed(proxy) {
    var empty = "";
    var box = empty.split("");
    var fields = box.push("-9223372036854775808");
    var input = vfs.localChannel("i", 1);
    var sent = input.trySend(7);
    var returned = proxy.combine(box, input);
    var copyback = box[0];
    var same = returned[0];
    same[0] = "9223372036854775807";
    var output = returned[1];
    var table = proxy.newtable();
    var tableOutput = proxy.update(table);
    var tableValue = table.getText("wide");
    var closedInput = input.close();
    var released = os.unload(proxy);
    var value = output.recv();
    var tableReply = tableOutput.recv();
    var closedOutput = output.close();
    var closedTable = tableOutput.close();
    return copyback + "|" + box[0] + "|" + value + "|" + tableValue + "|" + tableReply + "|" + released + "|";
}
