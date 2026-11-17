// Copyright (c) 2026 bhgv
// EN: Bind the existing IDE UI codec to a host-provided bidir terminal dispatcher.
// RU: Привязать существующий IDE UI codec к host-provided bidir terminal dispatcher.


// EN: Borrow terminal through all returned endpoint uses; return the ordinary UI codec or undefined.
// The host supplies bidir=true and type-routed trySendFrame/tryReceiveFrame operations.
// Calls remain nonblocking; false means not accepted, undefined means no incoming UI frame.
// This adapter does not consume other terminal types, perform network I/O or interpret S tags.
// RU: Borrow terminal до retirement endpoints; вернуть UI codec либо undefined.
// Host задаёт bidir=true и маршрутизированные nonblocking методы; чужие terminal types не читаются.
function makeIdeTerminalBridge(terminal)
{
    if (!terminal || terminal.bidir !== true || typeof terminal.trySendFrame != "function"
        || typeof terminal.tryReceiveFrame != "function")
    {
        return undefined;
    }
    var tx = new Object(); // Host-rooted UI-only outgoing capability.
    var rx = new Object(); // Host-rooted UI-only incoming capability.


    // EN: Borrow one existing fvd1 string; return true only after copied bounded acceptance.
    // RU: Borrow fvd1 string; true только после bounded copied acceptance.
    tx.trySend = function(frame)
    {
        return terminal.trySendFrame(frame) === true;
    };


    // EN: Return at most one routed fvd1 string, leaving every unrelated type to its dispatcher.
    // RU: Вернуть не более одной fvd1 строки; прочие types остаются dispatcher.
    rx.tryReceive = function()
    {
        return terminal.tryReceiveFrame();
    };
    return makeUiBridge(tx, rx);
}


// EN: Borrow terminal/target on their FlashVM owner; return the existing presentation adapter/undefined.
// Requires the existing makeIdePresentation implementation, no document/window policy moves here.
// RU: Borrow terminal/target на FlashVM owner; вернуть existing presentation adapter/undefined.
// Существующая makeIdePresentation обязательна; document/window policy здесь не меняется.
function makeIdeTerminalPresentation(terminal, target)
{
    var codec = makeIdeTerminalBridge(terminal); // Keep the existing IDE wire envelope.
    if (typeof codec == "undefined")
    {
        return undefined;
    }
    return makeIdePresentation(codec, target);
}
