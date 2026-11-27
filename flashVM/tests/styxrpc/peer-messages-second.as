// Copyright (c) 2026 bhgv
// EN: Distinguish the second exchange by the first byte of its response.
// RU: Отличить второй обмен по первому байту ответа.


// EN: Echo to the same port using the second root's verified schema.
// RU: Вернуть ответ тому же порту через проверенную схему второго корня.
function secondMessage(data, exchange) {
    data[0] = data[0] + 2;
    var result = exchange.send("peer.sample", 1, data);
}
