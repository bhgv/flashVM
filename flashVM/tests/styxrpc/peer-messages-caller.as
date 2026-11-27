// Copyright (c) 2026 bhgv
// EN: Open each published root from a different cwd and check its own reply.
// RU: Открыть оба корня из другого cwd и проверить ответ каждого.


// EN: Check hidden paths and root-local replies carrying the original binary sample.
// RU: Проверить скрытые пути и ответы со своим исходным двоичным образцом.
function onEnterFrame() {
    if (!messageStarted) {
        var ready = vfs.stat("/tools/message-ready");
        if (!ready[0]) {
            var hidden = os.stat("/color-input");
            var sibling = vfs.stat("/color-render");
            var cwd = vfs.cd("/tools");
            var schema = "peer.sample";
            var first = schema.split(",");
            var second = schema.split(",");
            var firstCount = first.push(1, 57, "firstMessageReply");
            var secondCount = second.push(1, 58, "secondMessageReply");
            messageFirst = os.openMessages("color-input", first);
            messageSecond = vfs.openMessages("color-render", second);
            var seed = "";
            var request = seed.split("");
            var size = request.push(40, 0, 255);
            var firstSent = messageFirst.send("peer.sample", 1, request);
            var secondSent = messageSecond.send("peer.sample", 1, request);
            messageStarted = true;
            messageHidden = hidden[0] + "," + sibling[0] + "," + vfs.pwd() + "," + firstSent + "," + secondSent;
        }
    }
    if (messageStarted) {
        if (!messageDone) {
            var firstPumped = messageFirst.pump(_root);
            var secondPumped = messageSecond.pump(_root);
            var observed = messageHidden + "|" + firstResult + "|" + secondResult + "|";
            if (!observed.indexOf("1,1,/tools,0,0|41,0,255|42,0,255|")) {
                var firstClosed = messageFirst.close();
                var secondClosed = messageSecond.close();
                var reported = _root.reportPeerResult();
                messageDone = true;
            }
        }
    }
}
