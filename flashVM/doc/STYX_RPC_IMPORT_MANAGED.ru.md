# fvm_styxrpc_import_managed

[English](STYX_RPC_IMPORT_MANAGED.md) · [Заголовок](../include/flashvm/styxrpc_managed.h) · [Реализация](../src/styxrpc/managed.c) · [Мост](STYX_RPC.ru.md)

Подключите `flashvm/styxrpc_managed.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_import_managed(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);
```

Зарегистрируйте глобальную функцию AS в выделенном контексте `context`
из потока владельца. В `target` укажите смонтированную сессию, относительный
путь модуля, имя экспорта, сигнатуру, полную схему и бюджет ожидания.
Для вызова с контекстом опубликуйте `target->pid` во встречном дереве Styx
и удерживайте публикацию на время вызова. Поля и статусы транспорта
описаны в [управляемом вызове](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/reference/managed_invoke.ru.md).

Поддерживаются строки Unicode с длиной, binary64 real, точные `int` и `byte`,
знаковый 64-битный `big`, плотные последовательности и кортежи в виде Array,
`null` для nil-строк и массивов и `undefined` для пустого результата.
В позиции `big` передавайте целый Number из точного диапазона или десятичный
String. Декодер возвращает Number в диапазоне ±(2⁵³−1) и каноническую строку
за его пределами. Формат и границы приведены в
[справочнике типов](STYX_RPC_VALUE_TYPES.ru.md#передача-знакового-64-битного-целого).
У последовательности читаются числовая длина и собственные индексные
свойства. При чтении свойств в потоке владельца могут исполняться геттеры.
Лимит последовательности — 2048 элементов, кадра — 64 КиБ. Длина кортежа
должна совпадать со схемой. Аргументы и результаты копируются по значению.
[Обычные аргументы ref](STYX_RPC_REFS.ru.md) поддерживают обратную запись.
Для вложенных и возвращаемых ref используйте [графовую привязку](STYX_RPC_IMPORT_GRAPH.ru.md).

[`FVM_OK`](../include/flashvm/flashvm.h#L44) означает успешную локальную регистрацию. Неверные параметры
дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Ошибки аргументов, профиля, удалённого
выполнения и неопределённого исхода передаются исключениями скрипта.
Ошибка преобразования результата возникает после удалённого вызова.
Она не даёт основания повторить вызов. Используйте поток владельца,
который может ждать. Храните target и строки, пока доступна функция.
Между вызовами владелец может обнулить сессию перед закрытием.

[Исполняемая проверка](../tests/styxrpc/managed.c) · [Экспорт](STYX_RPC_EXPORT_MANAGED.ru.md)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:260](../src/core/runtime.hpp#L260) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_import_managed` | function | [include/flashvm/styxrpc_managed.h:26](../include/flashvm/styxrpc_managed.h#L26) |
<!-- source-links:end -->
