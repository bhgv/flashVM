// Copyright (c) 2026 bhgv
// EN: Publish a callback with an invalid result to exercise peer refusal.
// RU: Проверить отказ клиента на недопустимом результате колбэка.


// EN: Return a result length beyond the borrowed extent.
// RU: Вернуть длину результата за границей заимствованного буфера.
function refuseNode(op, bytes, size) {
    return size + 1;
}
