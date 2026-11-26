// Copyright (c) 2026 bhgv
// EN: Transform a binary argument and count the use of this executable callback.
// RU: Преобразовать двоичный аргумент и посчитать вызовы колбэка.


// EN: Return a prefix with the call number and both ends of the argument.
// RU: Вернуть в префиксе номер вызова и крайние байты аргумента.
function applyNode(op, bytes, size) {
    _root.calls = _root.calls + 1;
    var written = 0;
    if (size) {
        var first = bytes[0];
        bytes[0] = _root.calls;
        bytes[1] = bytes[size - 1];
        bytes[2] = first;
        var code = op.charCodeAt(0);
        written = code - 117;
    }
    return written;
}
