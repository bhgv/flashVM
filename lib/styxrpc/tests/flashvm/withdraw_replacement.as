// Copyright (c) 2026 bhgv
// EN: Identify the replacement callback with a different returned byte.
// RU: Отличить новый колбэк по другому байту ответа.


// EN: Fill the first byte of the borrowed read or execute buffer.
// RU: Заполнить первый байт заимствованного буфера чтения или исполнения.
function withdrawReplacement(op, buf, size) {
    buf[0] = 83;
    return 1;
}
