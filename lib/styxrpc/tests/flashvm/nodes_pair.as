// Copyright (c) 2026 bhgv
// EN: Read, write and execute a live Limbo file through the default VFS.
// RU: Читать, записывать и исполнять живой файл Limbo через VFS по умолчанию.


// EN: Verify the shared directory, ordinary byte I/O and executable response once.
// RU: Один раз проверить общий каталог, обычный обмен байтами и ответ исполнения.
function onEnterFrame() {
    if (!nodeChecked) {
        var changed = os.cd("/tools");
        var directory = os.opendir(".");
        var entry = directory.next_file();
        var listed = 0;
        while (entry) {
            var name = entry.name + "|";
            if (!name.indexOf("mail|")) {
                listed = listed + 1;
            }
            entry = directory.next_file();
        }
        var listingClosed = directory.close();
        var stat = vfs.attr("mail");
        var rd = os.attr("ro");
        var wr = vfs.attr("wo");
        var file = vfs.open("mail", "rw");
        var clear = "";
        var bytes = clear.split("");
        var added = bytes.push(255, 0, 42, 23);
        var written = file.write(bytes);
        var reply = file.read(4);
        var read = reply.join(",");
        var closed = file.close();
        var call = clear.split("");
        var args = call.push(9, 0, 8);
        var result = os.exec("./mail", call);
        var returned = result.join(",");
        var line = listed + "|" + stat.mode + "|" + rd.mode + "|" + wr.mode + "|" + written + "|" + read + "|" + returned + "|";
        if (!line.indexOf("1|511|292|146|4|65,0|7,255|")) {
            var marker = clear.split("");
            var one = marker.push(91);
            var confirmed = vfs.exec("done", marker);
            nodeChecked = true;
        }
    }
}
