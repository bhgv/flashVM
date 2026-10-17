# Безопасность GC при вызовах нативных функций

Сборщик FlashVM принадлежит отдельному контексту; вызывать его параллельно
из другого системного потока нельзя. Вызовы C-функций, стековых функций,
вызываемых объектов и нативных метаметодов теперь защищены C++98 RAII-объектом.

- `fvm_gc_collect()` внутри такого вызова возвращает [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45).
- `fvm_gc_step()` не собирает объекты и устанавливает выходной счётчик в ноль.
- Вложенные вызовы учитываются счётчиком глубины; он восстанавливается при
  выходе, в том числе при перехвате исключений C++.
- Повторный запуск GC из финализатора `__gc` запрещён отдельно.

Защита от повторного запуска GC **не является** долговременным корнем объекта.
Если C-код хранит ссылку после возврата, используйте `fvm_gc_pin()` либо
`fvm_gc_roots_begin/add/end` с соблюдением порядка LIFO.

Для диагностики доступна `fvm_gc_stats()`. Перед вызовом установите поле
`struct_size` структуры [`FVM_GCStats`](../include/flashvm/gc.h#L70) равным `sizeof(FVM_GCStats)`.
Возвращаются количество объектов, закреплённых объектов, накопленных
выделений, порог GC, режим и глубина нативных вызовов.

## Ограничения

Это ещё не полностью завершённый сборщик мусора. Строки пока управляются
отдельно; контракты корней не формализованы для всех внешних вызовов.
Нужно дополнительно проверить автоматические безопасные точки AVM1 и
межпоточные сценарии. Один контекст нельзя использовать одновременно
из нескольких потоков. Заимствованный указатель может стать недействительным
после сборки.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../include/flashvm/gc.h#L48) |
| `fvm_gc_roots_begin` | function | [include/flashvm/gc_roots.h:27](../include/flashvm/gc_roots.h#L27) |
| `fvm_gc_stats` | function | [include/flashvm/gc.h:86](../include/flashvm/gc.h#L86) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `FVM_GCStats` | struct | [include/flashvm/gc.h:70](../include/flashvm/gc.h#L70) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
