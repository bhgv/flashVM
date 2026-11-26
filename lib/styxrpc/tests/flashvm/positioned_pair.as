// Copyright (c) 2026 bhgv
// EN: Publish a positioned file and exercise another file on DisVM through Styx.
// RU: Опубликовать файл со смещениями и проверить файл DisVM через Styx.


// EN: Verify metadata, absolute/current/end positions, EOF and independent fids before signalling.
// RU: Перед подтверждением проверить метаданные, смещения, EOF и независимые fid.
function onEnterFrame() {
    if (!positionedChecked) {
        var length = 2147483647 + 2147483647 + 10;
        var published = vfs.newnode("/view/file", "rwx", positionedTransfer, length);
        var path = "/tools/positioned";
        var file = os.open(path, "rw");
        var other = vfs.open(path);
        var lengthSeen = file.len();
        var info = vfs.attr(path);
        var writeAt = file.seek(0 - 2, "end");
        var clear = "";
        var input = clear.split("");
        var added = input.push(201, 0);
        var written = file.write(input);
        var eof = file.read(3);
        var readAt = file.seek(0 - 3, "cur");
        var tail = file.read(6);
        var tailText = tail.join(",");
        var first = other.read(1);
        var firstText = first.join(",");
        var closed = file.close();
        var otherClosed = other.close();
        var call = clear.split("");
        var packed = call.push(99);
        var result = os.exec(path, call);
        var value = result.join(",");
        var observed = lengthSeen + "|" + info.lengthHigh + "|" + info.lengthLow + "|" + writeAt + "|" + written + "|";
        observed = observed + eof.length + "|" + readAt + "|" + tailText + "|" + firstText + "|" + value + "|";
        if (!observed.indexOf("4294967304|1|8|4294967302|2|0|4294967301|11,201,0|7|88|")) {
            var done = vfs.exec("/tools/positioned-done");
            positionedChecked = true;
        }
    }
}
