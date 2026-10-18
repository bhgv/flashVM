# Трассирующий сборщик мусора FlashVM

## Принцип работы

Каждый [`FVM_Context`](../src/core/runtime.hpp#L240) владеет собственными объектами. Полный проход mark-and-sweep начинается с глобального объекта VM и объектов, закреплённых через [`fvm_gc_pin`](../include/flashvm/gc.h#L48). Обходятся именованные и числовые поля, прототипы и метатаблицы. **Циклические ссылки** между недостижимыми объектами не препятствуют их освобождению.

Сборщик не является параллельным. Вызывайте его в потоке владельца контекста, в безопасной точке выполнения. Пока стек интерпретатора AVM1 и временные значения C не добавлены в корни GC автоматически, нельзя запускать сборку посреди исполнения инструкций или нативного callback. Доступные сейчас проходы запускаются явно через API.

## Методы

- [`fvm_gc_pin`](../include/flashvm/gc.h#L48) / [`fvm_gc_unpin`](../include/flashvm/gc.h#L54): удержание внешних ссылок, допускается несколько закреплений.
- [`fvm_gc_collect`](../include/flashvm/gc.h#L44): полный проход, даже если установлен `suspended`.
- [`fvm_gc_mode`](../include/flashvm/gc.h#L37): число от -100 до 100, [`FVM_GC_SUSPENDED`](../include/flashvm/gc.h#L23) или [`FVM_GC_FULL`](../include/flashvm/gc.h#L26) (один полный проход без смены режима).
- [`fvm_gc_state`](../include/flashvm/gc.h#L40): получение сохранённого режима, 0 — normal/default.
- [`fvm_gc_step`](../include/flashvm/gc.h#L90): проверка порога и возможная сборка, не выполняется в suspended.
- [`fvm_gc_object_count`](../include/flashvm/gc.h#L57): количество объектов для диагностики.

Порог проверок равен `1000 - 9*mode` новых объектов: от 1900 при -100 до 100 при +100. Это простая политика по количеству выделений, не инкрементальный GC.

## Метаметод `__gc`

Перед освобождением недостижимого объекта вызывается его нативный обработчик `__gc`, которому передаются `(объект, метатаблица, объект)` и контекст. Обработчик может восстановить достижимость, записав объект в глобальную таблицу. Перед удалением выполняется повторная маркировка. Повторный вызов `__gc` для того же объекта не допускается.

**Ограничения:** Именованные AVM1-финализаторы, release userdata и объект `gc` доступны; см. `GC_COMPLETION.ru.md` и `ACTIONSCRIPT_EXTENSIONS.ru.md`. Указатели C на объекты без `pin` не считаются корнями, после удаления они недействительны.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:240](../src/core/runtime.hpp#L240) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `FVM_GC_FULL` | macro / define | [include/flashvm/gc.h:26](../include/flashvm/gc.h#L26) |
| `fvm_gc_mode` | function | [include/flashvm/gc.h:37](../include/flashvm/gc.h#L37) |
| `fvm_gc_object_count` | function | [include/flashvm/gc.h:57](../include/flashvm/gc.h#L57) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../include/flashvm/gc.h#L48) |
| `fvm_gc_state` | function | [include/flashvm/gc.h:40](../include/flashvm/gc.h#L40) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `FVM_GC_SUSPENDED` | macro / define | [include/flashvm/gc.h:23](../include/flashvm/gc.h#L23) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../include/flashvm/gc.h#L54) |
<!-- source-links:end -->
