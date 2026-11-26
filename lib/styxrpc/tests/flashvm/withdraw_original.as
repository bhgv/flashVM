// Copyright (c) 2026 bhgv
// EN: Serve the original file and arm withdrawal after the current request.
// RU: Обслужить первоначальный файл и запланировать отзыв после текущего запроса.


// EN: Return byte 17 or test that the owner cannot retire the current callback from inside it.
// RU: Вернуть байт 17 или проверить, что владелец не может отозвать работающий колбэк.
function withdrawOriginal(op, buf, size) {
    var code = op.charCodeAt(0);
    var result = 1;
    if (code - 119) {
        buf[0] = 17;
    } else {
        var removed = os.rmnode("/menu/data");
        if (removed) {
            result = 0;
        } else {
            _root.withdrawArmed = true;
        }
    }
    return result;
}
