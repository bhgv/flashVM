// Copyright (c) 2026 bhgv
// EN: Retain the binary reply from the first exchange to verify it later.
// RU: Сохранить двоичный ответ первого обмена для проверки.


// EN: Join a private message byte array into the expected decimal record.
// RU: Соединить отдельный массив байтов сообщения в запись для сравнения.
function firstMessageReply(data) {
    _root.firstResult = data.join(",");
}
