// Copyright (c) 2026 bhgv
// EN: Call an executable Limbo node and acknowledge the returned bytes.
// RU: Вызвать исполняемый узел Limbo и подтвердить полученные байты.


// EN: Verify the empty and counted replies before sending the completion marker.
// RU: Проверить пустой ответ и ответ с длиной до отправки подтверждения.
function onEnterFrame() {
    if (!publishedChecked) {
        var changed = os.cd("/tools");
        var empty = vfs.exec("apply");
        var emptySize = empty.length;
        var clear = "";
        var input = clear.split("");
        var size = input.push(255, 40, 0, 23);
        var result = os.exec("./apply", input);
        var text = result.join(",");
        var original = input.join(",");
        var line = emptySize + "|" + text + "|" + original + "|";
        var large = clear.split("");
        var inserted = large.push(17);
        var count = 1;
        while (16384 - count) {
            large = large.concat(large);
            count = count * 2;
        }
        var returned = vfs.exec("apply", large);
        var length = returned.length;
        var first = returned[0];
        var last = returned[16383];
        var untouched = large[0];
        var resultLine = line + length + "|" + first + "|" + last + "|" + untouched + "|";
        if (!resultLine.indexOf("0|23,0,255|255,40,0,23|16384|88|99|17|")) {
            var marker = clear.split("");
            var one = marker.push(207);
            var confirmed = vfs.exec("/tools/done", marker);
            publishedChecked = true;
        }
    }
}
