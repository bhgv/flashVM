# Опрос callback и отмена

Исполняемый пример C89: реальные часы контекста, native Function и globals
таймеров AS1/AS2. SWF, сеть, дисплей и worker не нужны. [English](README.md).
[Полный контракт](../../../doc/TIMERS.ru.md).

Сборка/запуск из компонента FlashVM:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_timers_main
./build/flashvm_examples_embedding_timers_main
./build/flashvm_examples_embedding_timers_main --wait
```

По умолчанию один poll и отмена без ожидания. `--wait` просит Enter: подождать
хотя бы секунду, чтобы interval стал due. Вывод:
`cancelled interval; callbacks: N; attempted: N`, обычно N=0 по умолчанию и N=1
интерактивно. Медленная установка/быстрый ввод меняют готовность; за poll этот
interval всё равно вызывается не более одного раза. CTest запускает обычный режим.

## API, время жизни и ожидания

| Функция | Назначение в примере |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Инициализировать принадлежащую вызывающему конфигурацию ABI. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Скопировать конфигурацию и получить runtime, живущий дольше контекста. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Создать heap на потоке владельца и независимое начало отсчёта. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L19) | Транзакционно добавить globals, сохраняя конфликтующие собственные имена. |
| [fvm_function_create_native](../../../include/flashvm/function.h#L49) | Создать managed Function с заимствованными кодом/userdata; pending-регистрация удержит её. |
| [fvm_value_undefined](../../../include/flashvm/value.h#L40) | Инициализировать поля Function-значения или игнорируемый результат callback без буфера. |
| [fvm_value_number](../../../include/flashvm/value.h#L42) | Подготовить числовую задержку без управляемой памяти. |
| [fvm_call](../../../include/flashvm/value.h#L138) | Синхронно зарегистрировать/отменить через setInterval/clearInterval; возможны память/coercion, ожидания deadline нет. |
| [fvm_timers_poll](../../../include/flashvm/timers.h#L19) | Синхронно доставить due-снимок с user code/выделением; счётчик включает ошибочную зафиксированную доставку. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Снять pending-корни и освободить heap до выхода host-счётчиков из области жизни. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | Освободить внешнего владельца после уничтожения контекстов. |

Локальные счётчики живут до завершения pending и активных вызовов. Pin не нужен:
между созданием Function и регистрацией нет GC, затем её удерживает очередь.
Числовые ID не владеют буфером. Общая ветка очистки снимает и оставшуюся pending
регистрацию при ошибке. Все вызовы VM идут на потоке владельца. Ждёт только
опциональный host `getchar()`; VM не спит и не создаёт timer thread.
Обычный native callback сохраняет GC-барьер. Пример не подтверждает точное
расписание оригинального Player, воспроизведение на устройстве или другую ОС.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../../../include/flashvm/value.h#L138) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_function_create_native` | function | [include/flashvm/function.h:49](../../../include/flashvm/function.h#L49) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:19](../../../include/flashvm/standard.h#L19) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:19](../../../include/flashvm/timers.h#L19) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
