// Copyright (c) 2026 bhgv
// EN: Exercise ordinary OS handles and synchronous SWF loading from the live VFS.
// RU: Проверить обычные дескрипторы OS и загрузку SWF из живой VFS.


// EN: Mark success only after the byte cursor and loaded display object work.
// RU: Отметить успех после проверки байтового курсора и объекта фильма.
function onEnterFrame() {
    if (!osDone) {
        entries = os.opendir("/os-assets");
        first = entries.next_file();
        second = entries.next_file();
        eof = entries.next_file();
        dirClosed = entries.close();
        fileAttr = os.attr("/os-assets/marker.txt", "/");
        dirAttr = os.attr("/os-assets");
        file = os.open("/os-assets/marker.txt", "rw");
        length = file.len();
        original = file.read(1);
        originalText = original.join(",");
        end = file.seek(0 - 1, "cur");
        childFile = os.open("/os-assets/child.swf");
        child = childFile.load();
        childClosed = childFile.close();
        depth = child.getDepth();
        if (first) {
            if (second) {
                if (!eof) {
                    if (fileAttr) {
                        if (dirAttr) {
                            if (!(length - 1)) {
                                if (!originalText.indexOf("65")) {
                                    if (!end) {
                                        payloadText = "";
                                        payload = payloadText.split("");
                                        payloadCount = payload.push(90);
                                        written = file.write(payload);
                                        confirmed = file.flush();
                                        fileClosed = file.close();
                                        osDone = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
