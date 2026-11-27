// Copyright (c) 2026 bhgv
// EN: Exercise the same wide integers through by-value arrays, ref copy-back and channel leases.
// RU: Проверить те же числа через массивы по значению, обратную запись ref и аренду канала.


// EN: Borrow a typed Dis proxy, leaving the original by-value arrays and their contents unchanged.
// RU: Заимствовать типизированный прокси Dis, сохранив исходные массивы и их данные.
function proveWideValues(proxy) {
    var empty = "";
    var words = empty.split("");
    var wordCount = words.push("9223372036854775807", "text");
    var values = empty.split("");
    var valueCount = values.push("+09223372036854775807", "-9223372036854775808", 42, "-9007199254740992");
    var copied = proxy.wide(words, values);
    var returnedWords = copied[0];
    var returnedValues = copied[1];
    var proof = returnedWords[0] + "|" + returnedWords[1] + "|" + returnedValues[0] + "|" +
        returnedValues[1] + "|" + returnedValues[2] + "|" + returnedValues[3] + "|" +
        words[1] + "|" + values[0] + "|";
    var answer = "invalid";
    if (!proof.indexOf("9223372036854775807|copy|9223372036854775806|-9223372036854775808|42|-9007199254740992|text|+09223372036854775807|")) {
        var box = empty.split("");
        var fields = box.push("-9223372036854775808");
        var same = proxy.patch(box, "9223372036854775807");
        var patched = box[0] + "|";
        if (!patched.indexOf("9223372036854775807|")) {
            same[0] = "-9223372036854775808";
            var alias = box[0] + "|";
            if (!alias.indexOf("-9223372036854775808|")) {
                var queue = vfs.localChannel("a[b]", 1);
                var outgoing = empty.split("");
                var count = outgoing.push("9223372036854775807", "-9223372036854775808");
                var sent = queue.trySend(outgoing);
                var reply = proxy.widepipe(queue);
                var incoming = reply.recv();
                var closeReply = reply.close();
                var closeQueue = queue.close();
                answer = incoming[0] + "|" + incoming[1];
            }
        }
    }
    return answer;
}
