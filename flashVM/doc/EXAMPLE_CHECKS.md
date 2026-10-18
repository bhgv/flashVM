# Tutorial checks and parameters / Проверки и параметры примеров — v0.21

## English

All 161 assertions in 16 tutorial programs are now explicit `if` checks.
Each FlashVM call is evaluated once, including when `NDEBUG` is defined.
The former `assert(fvm_...())` construction could otherwise remove the API call
when an example was compiled independently with assertions disabled.

Comments explain the real API operation and its arguments, rather than the
assertion mechanism. Worker/channel examples distinguish configuration, callback,
input, borrowed userdata, returned worker handles and retained channel wrappers.
The callback userdata is not copied and must remain valid until the worker joins;
a channel copy retains the same queue rather than duplicating it.

`examples/example_support.h` reports the failed condition, source file and line,
then exits the entire tutorial process with `EXIT_FAILURE`. This deliberately
matches the previous examples' fail-fast approach; it does not implement error-path
resource cleanup or unwind C++ owners. Successful runs retain their explicit joins
and releases. Applications should propagate errors and perform appropriate cleanup.
Tests continue using their existing assertion framework.

Verification: all 35 C/C++ tutorial programs passed with normal build flags and
when compiled with `-DNDEBUG -O2` against the bundled AGG/font/codec/SQLite libraries.
An independent syntax-tree comparison preserved all 414 FlashVM API calls and their
argument expressions. Comments and control-flow explanations were also reviewed in
ActionScript samples; source compilation of AS design targets is still unavailable.
No external library, existing test or build script was reformatted or commented.

## Русский

161 `assert` в 16 учебных программах заменён явными проверками `if`.
Каждый вызов FlashVM выполняется ровно один раз, в том числе при `NDEBUG`.
При самостоятельной сборке прежний `assert(fvm_...())` мог убрать сам вызов API.

Комментарии описывают вызываемую функцию и её параметры. Для потоков и каналов
поясняются настройки VM, callback, начальное значение, заимствованный userdata,
выходной дескриптор потока и удерживаемая обёртка канала. Userdata не копируется
и должен жить до join; копия обёртки канала удерживает ту же очередь сообщений.

Общий учебный обработчик печатает нарушенное условие, исходный файл и строку,
затем завершает весь процесс с `EXIT_FAILURE`. Как и прежние assert, это аварийное
завершение примера: оно не выполняет очистку ресурсов и раскрутку C++ владельцев.
Успешные пути по-прежнему явно присоединяют потоки и освобождают ресурсы. В реальных
приложениях ошибки следует передавать вызывающей стороне и выполнять очистку.
Существующие тесты сохраняют прежние assert.

Все 35 C/C++ примеров прошли с обычными флагами и с `-DNDEBUG -O2`; использованы
статические библиотеки профиля AGG со шрифтами, кодеками и SQLite. Сравнение AST
подтвердило сохранение всех 414 вызовов API и выражений аргументов. Комментарии AS
также уточнены; компиляция исходных AS design targets ещё не реализована.
Внешние библиотеки, существующие тесты и скрипты сборки не оформлялись заново.
