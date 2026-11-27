# Styxmodref.capture

[English](capture.md) · [Объявление](../../module/styxmodref.m) · [Реализация](../../limbo/styxmodref.b) · [Руководство](../STYX-MODULE-REFS.ru.md)

`styxmodref.dis`

```limbo
capture: fn(path: string, writable: int): ref Cell;
```

Передайте канонический относительный путь, например `mounted/ref-pair.dis/.var/counter`, и `writable` равный 0 или 1. Путь отсчитывается от корня исходной подключённой FlashVM. Вызов открывает экспорт, проверяет точный fid и возвращает собственный `ref Cell`. Он может ждать обработки открытия и stat. Неверный путь или режим, отсутствие службы, отказ доступа или неверный тип файла вызывают исключение. Читайте через [Cell.read](read.ru.md) либо отправляйте дескриптор из [Cell.payload](payload.ru.md).

В паре с FlashVM родитель может прервать удалённый захват прямого потомка через `Processctl.cancelwait(pid)`. Прерванный захват завершится исключением.
[Исполняемый пример Limbo и FlashVM](../../../lib/styxrpc/examples/disvm/REF_PORT_CANCELLATION.ru.md).
