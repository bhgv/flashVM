// Copyright (c) 2026 bhgv
// EN: Publish composed queues and drive two typed result endpoints in the stock player.
// RU: Публикация составных очередей и работа двух концов результата в штатном плеере.


// EN: Return one argument ref, its updates and three channel leaves.
// RU: Вернуть ссылку аргумента, её изменения и три листа канала.
function compose(box, option, absent) {
    box[0] = box[0] + 5;
    var empty = "";
    var ports = empty.split("");
    var result = empty.split("");
    var labelPort = absent;
    if (option - 2) {
        var numbers = empty.split("");
        var count = numbers.push(71);
        var slots = 1;
        if (option) {
            slots = 9;
        }
        var numberPort = empty.split("");
        var portItems = numberPort.push(slots, numbers);
        var n = ports.push(numberPort, numberPort, absent);
        var labels = empty.split("");
        var k = labels.push("label");
        labelPort = empty.split("");
        var textItems = labelPort.push(1, labels);
    } else {
        var one = ports.push(absent);
    }
    var length = result.push(box, ports, labelPort);
    return result;
}
