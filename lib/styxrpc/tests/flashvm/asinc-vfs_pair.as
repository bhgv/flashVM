// Copyright (c) 2026 bhgv
// EN: Launch once, then publish completion only after verifying independent cwd.
// RU: Запустить задачу один раз, подтвердить завершение после проверки отдельного cwd.
function onEnterFrame() {
    if (!fileTask) {
        fileTask = _root.readFromVfs("input");
    }
    var result = fileTask.poll();
    if (result.done) {
        if (!result.status) {
            if (!(result.value - 42)) {
                var cwd = vfs.pwd() + "|";
                if (!cwd.indexOf("/|")) {
                    if (!completed) {
                        completed = vfs.mkdir("/ack");
                    }
                }
            }
        }
    }
}
