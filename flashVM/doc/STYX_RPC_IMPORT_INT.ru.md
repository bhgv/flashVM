# fvm_styxrpc_import_int

[English](STYX_RPC_IMPORT_INT.md) · [Объявление](../include/flashvm/styxrpc.h) · [Реализация](../src/styxrpc/scalar.c) · [Подключение моста](STYX_RPC.ru.md)

Подключите `flashvm/styxrpc.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_import_int(FVM_Context *context, const char *global,
    FVM_StyxRpcImport *binding);
```

Из потока владельца зарегистрируйте глобальное имя `global` в `context`.
Передайте выделенный приложением `binding` со смонтированной сессией, относительным
путём модуля, именем экспорта и временем опроса готовности ответа 1–60000 мс.
[`FVM_OK`](../include/flashvm/flashvm.h#L44) означает, что импорт зарегистрирован. Удалённый профиль проверяется при
каждом вызове. Отсутствующие или неверные локальные параметры дают
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Ошибки выделения памяти возвращает FlashVM.

Функция скрипта принимает один точный Int32 и возвращает точный Int32 как
число. Неверный аргумент, отказ до отправки, подтверждённая удалённая ошибка
и неопределённый исход дают разные исключения. Вызов автоматически не
повторяется. Помимо времени опроса действуют сроки I/O сессии Styx.
Исполняйте вызов в выделенном потоке владельца, который может ждать.
Храните binding и строки, пока существует вызываемая ссылка на эту функцию.
Между вызовами владелец может обнулить `binding->session` перед закрытием
сессии. Следующие вызовы сообщат, что запрос не отправлен.

[Исполняемая проверка](../tests/styxrpc_host.c) · [Экспорт](STYX_RPC_EXPORT_INT.ru.md)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_import_int` | function | [include/flashvm/styxrpc.h:38](../include/flashvm/styxrpc.h#L38) |
| `FVM_StyxRpcImport` | struct | [include/flashvm/styxrpc.h:17](../include/flashvm/styxrpc.h#L17) |
<!-- source-links:end -->
