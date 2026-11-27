// Copyright (c) 2026 bhgv
// EN: Retain the binary reply from the second exchange to verify it later.
// RU: Сохранить двоичный ответ второго обмена для проверки.


// EN: Join a private message byte array into the expected decimal record.
// RU: Соединить отдельный массив байтов сообщения в запись для сравнения.
function secondMessageReply(data) {
    _root.secondResult = data.join(",");
}
