// Copyright (c) 2026 bhgv
// EN: Poll the channel worker without waiting on the movie owner.
// RU: Опросить задачу канала без ожидания у владельца фильма.
function onEnterFrame() {
    if (!channelTask) {
        channelTask = _root.useChannel("/work", false);
    }
    var result = channelTask.poll();
    if (result.done) {
        if (!result.status) {
            var value = result.value;
            if (!value.indexOf("typed|")) {
                var reported = _root.reportPeerResult();
            }
        }
    }
}
