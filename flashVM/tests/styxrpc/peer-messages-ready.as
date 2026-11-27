// Copyright (c) 2026 bhgv
// EN: Expose a regular file after both service-only roots become live.
// RU: Опубликовать обычный файл, когда оба служебных корня доступны.


// EN: Return EOF. The client uses only the file's existence.
// RU: Вернуть EOF. Клиент проверяет только существование файла.
function messageReadyFile(operation, data, size) {
    return 0;
}
