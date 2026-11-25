// Copyright (c) 2026 bhgv
// EN: Handle the registered UniBus credit on the movie's owner.
// RU: Обработать разрешение UniBus у владельца фильма.


// EN: Distinguish the credit callback from the frame callback.
// RU: Отдельно от колбэка кадра учесть колбэк разрешения.
function onPeerCredit() {
    peerReceivedCredit = true;
}
