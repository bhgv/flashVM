// Copyright (c) 2026 bhgv
// EN: Publish composed queues and drive two typed result endpoints in the stock player.
// RU: Публикация составных очередей и работа двух концов результата в штатном плеере.


// EN: Register source-derived Limbo recipes and service owner I/O between calls.
// RU: Установить схемы Limbo и обслуживать ввод-вывод между вызовами.
function onEnterFrame() {
    if (!published) {
        var composeSpec = "compose,compose";
        var bufferSpec = "buffered,buffered";
        var meetingSpec = "meeting,meeting";
        var retiredSpec = "retired,retired";
        var a = composeSpec.split(",");
        var b = bufferSpec.split(",");
        var c = meetingSpec.split(",");
        var d = retiredSpec.split(",");
        var compiled = 2000000000 + 1122202711;
        var ac = a.push(compiled, "(r[i]is)", "((r[i]a[c[i]]c[s]))", "channel", 0);
        var bc = b.push(1470161458, "(s)", "(c[s])", "channel", 1);
        var cc = c.push(166697325, "(i)", "(c[i])", "channel", 0);
        published = vfs.publishObject("channel-results.dis", _root, a, b, c, d);
    }
    if (bufferedPort) {
        if (bufferedPort.active()) {
            var incoming = bufferedPort.tryReceive();
            if (incoming) {
                var accepted = bufferedPort.trySend("echo:" + incoming);
            }
        }
    }
    if (meetingPort) {
        if (meetingPort.active()) {
            if (!sentMeeting) {
                if (!pendingMeeting) {
                    pendingMeeting = meetingPort.tryReceive();
                }
                if (pendingMeeting) {
                    sentMeeting = meetingPort.trySend(pendingMeeting + increment);
                }
            }
        }
    }
}
