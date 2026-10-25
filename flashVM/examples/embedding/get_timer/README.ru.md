# Часы контекста через getTimer

Исполняемый пример C89: установка стандартной библиотеки и чтение getTimer
AS1/AS2 без SWF, устройства, сети, worker и задержки. [English](README.md).
[Контракт часов](../../../doc/GET_TIMER.ru.md).

Из компонента FlashVM:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_get_timer_main
./build/flashvm_examples_embedding_get_timer_main
```

Результат: `context elapsed milliseconds: N`, где N — целое неотрицательное
число меньше 2^32. Точное значение зависит от платформы и времени установки.
Ошибка часов печатается с ненулевым кодом выхода.

## API, владение и блокировки

| Функция | Назначение в примере |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L70) | Инициализировать принадлежащую вызывающему конфигурацию ABI. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L74) | Скопировать конфигурацию и получить внешний runtime. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L82) | Получить heap на потоке владельца и независимое начало отсчёта. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L22) | Опубликовать getTimer и стандартные объекты; сохранить конфликтующие имена. |
| [fvm_call](../../../include/flashvm/value.h#L204) | Синхронно прочитать Number через getTimer без ожидания и callback; сообщить ошибку часов/памяти. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L86) | Освободить heap и стандартные объекты до уничтожения runtime. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L78) | Освободить внешнего владельца после всех его контекстов. |

Числовой результат не владеет буфером. Все вызовы — на потоке владельца.
Установка и обвязка вызова могут выделять память; callback часов не выделяет
память, не преобразует аргументы и не ждёт. Начало отсчёта — создание контекста,
переполнение unsigned по модулю 2^32. Совпадение этих правил с оригинальным
проигрывателем ещё не принято. Интервал и timeout не создаются; восстановление
часов и ограничения описаны в связанном контракте.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:204](../../../include/flashvm/value.h#L204) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
<!-- source-links:end -->
