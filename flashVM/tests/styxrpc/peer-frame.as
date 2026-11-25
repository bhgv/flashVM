// Copyright (c) 2026 bhgv
// EN: Handle a registered UniBus reply on the same owner as the AS movie.
// RU: Обработать зарегистрированный ответ UniBus у владельца фильма.


// EN: Record that the callback registered for the frame was called.
// RU: Зафиксировать вызов колбэка, зарегистрированного для кадра.
function onPeerFrame() {
    peerReceivedFrame = true;
}
