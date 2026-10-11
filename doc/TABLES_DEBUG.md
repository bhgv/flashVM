# Tables and debugger inspection / Таблицы и инспекция отладчика

## English

`flashvm/table.h` introduces **named properties plus sparse unsigned-long indexed slots** on the same VM-owned object. Indexed slots use a C++98 ordered map internally for now, **not** a Lua-style hybrid hash/array/queue allocator. A separate metatable may supply missing *indexed* entries; the AVM1 prototype chain continues to govern named-property lookup. Cycles and cross-context metatables are rejected. This is a staged API, **not** Lua's `__index`, `__newindex`, or full metamethod protocol. The queue part is not implemented.

`flashvm/debug.h` exposes read-only enumeration of allocated objects and registered native/script function names. `fvm_table_named_at` and `fvm_table_indexed_at` enumerate own properties. Iteration order is implementation-defined and enumeration is invalidated by mutation. Pointers and names are borrowed until mutation or context destruction. Do not call concurrently with VM execution.

### Example

See `examples/embedding/tables_debug/main.c`: create two tables, attach a metatable, set numeric slots, enumerate variables and inspect active allocations.

### Next architecture steps

Function arguments and locals still use existing interpreter data structures; replacing these with distinct table instances, passing replaceable hidden environment tables, making movie scripts callable closures, exposing table-based native arguments, and adding single-step execution require separate VM changes. A context is already the first argument of C native callbacks, but simultaneous execution of a single context is unsupported. Multiple contexts do not imply safe cross-thread object sharing.

## Русский

`flashvm/table.h` добавляет к объектам VM **именованные свойства и разреженные числовые слоты** (`unsigned long`). Пока внутри применяется упорядоченный `std::map` C++98, а **не** гибридная структура Lua «хэш + массив + очередь». Отдельная метатаблица обеспечивает поиск отсутствующих *числовых* элементов. Для именованных свойств продолжает действовать цепочка прототипов AVM1. Циклы и метатаблицы другого контекста запрещены. Метаметоды Lua `__index`/`__newindex` и очередь пока отсутствуют.

`flashvm/debug.h` предоставляет перечисление созданных объектов, имён нативных и скриптовых функций. Функции `fvm_table_named_at` и `fvm_table_indexed_at` перечисляют собственные элементы таблицы. Порядок не гарантируется; при изменении таблицы перечисление необходимо начинать заново. Указатели и строки заимствованные. Одновременный вызов из другого потока во время выполнения VM не допускается.

### Пример

`examples/embedding/tables_debug/main.c` показывает создание таблиц, установку метатаблицы, запись числовых элементов и перечисление переменных.

### Что ещё не готово

Локальные переменные и параметры функций пока не переведены на отдельные таблицы. Скрытые заменяемые пространства имён, функции-замыкания для MovieClip, передача аргументов C-функциям в виде таблицы и пошаговый отладчик требуют отдельной реализации. Контекст уже передаётся первым параметром нативного callback. Одновременное выполнение одного контекста в нескольких потоках не поддерживается.
