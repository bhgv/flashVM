// Copyright (c) 2026 bhgv
// EN: Serve the positioned FlashVM node used by the paired Limbo client.
// RU: Обслужить узел FlashVM со смещениями для парного клиента Limbo.


// EN: Apply an absolute offset to the tail of the exposed byte range and preserve binary zeros.
// RU: Использовать абсолютное смещение в конце диапазона с сохранением нулевых байтов.
function positionedTransfer(op, buf, count, offset) {
    var used = 0 - 1;
    var key = op + "|" + offset + "|" + count + "|";
    if (!key.indexOf("w|4294967302|2|")) {
        _root.flashTail0 = buf[0];
        _root.flashTail1 = buf[1];
        used = 2;
    }
    if (!key.indexOf("r|4294967301|3|")) {
        buf[0] = 33;
        buf[1] = _root.flashTail0;
        buf[2] = _root.flashTail1;
        used = 3;
    }
    if (!key.indexOf("r|0|1|")) {
        buf[0] = 5;
        used = 1;
    }
    if (!key.indexOf("x|0|1|")) {
        buf[0] = buf[0] + 1;
        used = 1;
    }
    return used;
}
