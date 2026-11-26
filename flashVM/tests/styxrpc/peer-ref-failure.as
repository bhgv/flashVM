// Copyright (c) 2026 bhgv
// EN: Trigger a rejected alias assignment in a compiled test SWF.
// RU: Вызвать отклонение привязки ссылки в собранном проверочном SWF.


// EN: Bind one alias and then attempt to bind a second reference into that same slot.
// RU: Привязать ссылку и затем попытаться записать вторую ссылку в занятый слот.
function onEnterFrame() {
    var counter = 41;
    var refs = channel.create("ref", 1);
    var sent = refs.send(ref counter);
    var alias = refs.recv();
    var sentAgain = refs.send(ref counter);
    alias = refs.recv();
}
