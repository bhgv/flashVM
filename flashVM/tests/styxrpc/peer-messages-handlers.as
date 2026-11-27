// Copyright (c) 2026 bhgv
// EN: Change the first byte differently on each root while preserving the rest.
// RU: По-разному изменить первый байт в каждом корне, сохраняя остальные.


// EN: Echo to the same port using the first root's verified schema.
// RU: Вернуть ответ тому же порту через проверенную схему первого корня.
function firstMessage(data, exchange) {
    data[0] = data[0] + 1;
    var result = exchange.send("peer.sample", 1, data);
}
