# Native Array length reentry and GC / Повторный вход и GC — v0.529

Exact loaded `length` assignment now prepares the original native accessor too.
Conversion runs once before selecting the live descriptor. A valueOf callback may
delete that accessor or replace it with a script virtual; the incoming typed
value is published into the resulting own slot/descriptor. Truncation reads the
post-callback raw data, so deleting length removes its old numeric bound while
virtual replacement retains captured numeric data. Exceptions preserve callback
mutations but do not publish the incoming value. Reinstalled native accessors
reuse the prepared conversion. The private direct-setter callback retains its
explicit conversion path; public C/interpreter/JIT property writes share the
live dispatch path.

[GC:] Deleting the effective native length accessor clears its obsolete cached
value to a metadata-free Number zero. This is private fallback storage, not a
recreated public property. The receiver can no longer keep deleted objects,
owned strings or primitive-metatable cycles alive through a hidden cache edge.
Clearing occurs only after successful flag/name checks; failed or nondeletable
removal preserves the cache. Native callback aliases are not mistaken for the
effective length slot. Virtual replacement captures the old data before clearing
the cache; ordinary shared property edges continue to retain the graph normally.

Точное присваивание length теперь подготавливает и штатный native-accessor до
выбора живого дескриптора. При удалении/замене length внутри valueOf значение
сохраняется в актуальное собственное свойство/виртуальный дескриптор, без второго
преобразования. Усечение использует данные после callback. Ошибка сохраняет
побочные эффекты callback, но не публикует входное значение.

[GC:] После успешного удаления штатной length её неактуальный внутренний кэш
заменяется Number 0 без метатаблицы; публичное свойство не пересоздаётся.
Удалённые объекты/циклы больше не удерживаются скрытым полем. Нельзя удалить
свойство — кэш сохраняется. Другой alias того же C-callback не считается
штатной length. Виртуальный дескриптор получает прежние данные до очистки;
общие ссылки из других свойств продолжают удерживать граф.

Tests / проверки:

- `array_length_cache_gc`: SWF4–8, nine nonnumeric inputs and numeric metadata,
  folded deletion, virtual replacement, alias control and independent shared
  edges; actual GC must release the obsolete graph only after its last live edge.
- `array_native_length_reentry`: original native length deletion/replacement
  during collecting valueOf, errors/throws, C API/interpreter/explicit/automatic
  JIT, retained virtual data, visible own publication and single conversion.
- Existing typed, recreated, raw-data, allocation, transport and execution tests
  remain active. Native reinstatement fixtures now explicitly restore their
  intended Number bound instead of relying on obsolete deleted cache data.

Validation on Linux x64 C89/C++98: all 1095 minimal CTest cases passed after
restoring execute bits on nine failed generated-ELF launches and rerunning them.
The final shared-edge GC control was rebuilt and passed separately. Nine targeted
ASan/UBSan cases passed, including both allocation sweeps, with stack checks
active and LeakSanitizer disabled. No generated logs/reports are committed.
The ABI and wire layout are unchanged. Full Array/SWF compatibility, general
case/path semantics, all target OS runs and all nine roadmap stages remain open.
