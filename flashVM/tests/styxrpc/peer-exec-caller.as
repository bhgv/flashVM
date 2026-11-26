// Copyright (c) 2026 bhgv
// EN: Invoke the other FlashVM's executable callback using the paired session.
// RU: Вызвать исполняемый колбэк другой FlashVM по штатной парной сессии.


// EN: Verify shared cwd, capacity-preserving binary calls and independent replies.
// RU: Проверить общий cwd, двоичные вызовы и независимые массивы ответа.
function onEnterFrame() {
    if (!checkedExec) {
        var metadata = vfs.stat("/tools/apply");
        if (!metadata[0]) {
            var changed = os.cd("/tools");
            var dir = vfs.pwd();
            var empty = os.exec("apply");
            var emptySize = empty.length;
            var seed = "";
            var request = seed.split("");
            var requestSize = request.push(255, 40, 0, 23);
            var reply = vfs.exec("./apply", request);
            var again = os.exec("/tools/../tools/apply", request);
            var first = reply.join(",");
            var second = again.join(",");
            var original = request.join(",");
            var record = dir + "|" + emptySize + "|" + first + "|" + second + "|" + original + "|";
            if (!record.indexOf("/tools|0|2,23,255|3,23,255|255,40,0,23|")) {
                var reported = _root.reportPeerResult();
                checkedExec = true;
            }
        }
    }
}
