# fvm_worker_run_function_args

[English](WORKER_FUNCTION_ARGS.md) · [Запуск функции в потоке](WORKER_FUNCTION.ru.md) ·
[Объявление](../include/flashvm/worker.h) · [Реализация](../src/capi/worker_function.cpp)

Подключите `flashvm/worker.h`. Передайте контекст владельца, конфигурацию
и функцию AVM1 как в [`fvm_worker_run_function`](../include/flashvm/worker.h#L38). Укажите `count` позиционных
значений в заимствованном массиве `args`, от нуля до
[`FVM_WORKER_FUNCTION_INPUTS`](../include/flashvm/worker.h#L42). При нуле указатель может быть пустым.
Замыкание и аргументы сериализуются вместе в потоке владельца.
Повторные ссылки на сериализуемый объект указывают на одну копию в потоке.

При успехе `out` получает собственный дескриптор системного потока.
Дождитесь его завершения перед уничтожением дескриптора. При ошибке
`out` очищается. Запуск может отклонить неверное число или значения,
нативные ресурсы, которые нельзя скопировать, и отказ кодека или создания
потока. Ошибки исполнения и примитивные результаты возвращаются по обычному
контракту потока.

Передайте `0, NULL` для вызова без аргументов. В ActionScript тот же
интерфейс доступен через [Async.run](ASINC_RUN.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_WORKER_FUNCTION_INPUTS` | macro / define | [include/flashvm/worker.h:42](../include/flashvm/worker.h#L42) |
| `fvm_worker_run_function` | function | [include/flashvm/worker.h:38](../include/flashvm/worker.h#L38) |
| `fvm_worker_run_function_args` | function | [include/flashvm/worker.h:53](../include/flashvm/worker.h#L53) |
<!-- source-links:end -->
