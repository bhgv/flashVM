# Учёт и время жизни строк v0.68 / String storage and lifetime

Контекст хранит string buffers вместе с точной payload byte length.
Регистрация descriptor и address index атомарна: при отказе одного из
контейнеров caller сохраняет ownership для cleanup. Декодер удаляет index
перед rollback/free, поэтому повторное использование адреса не создаёт
ложное владение. Context destruction освобождает каждый buffer один раз.

`fvm_gc_string_stats` возвращает buffers, payload_bytes и allocated_bytes.
Последнее включает payload + final terminator для каждого buffer. Метаданные
контейнеров, allocator overhead, строки внешних payloads/ресурсов и process RSS
в эти числа не входят. UTF-8/WTF-8 хранится в байтах: embedded NUL и одиночные
суррогаты не теряют длину через strlen. Query не выделяет память и не запускает
callbacks; ошибка не изменяет caller output.

Нативные getters/methods/__index и value metamethod results сохраняют
возвращённые внешние/resource-owned byte views в context storage до снятия
native roots. Уже зарегистрированный buffer переиспользуется без копирования.
Это устраняет зависимость возвращённой строки от финализации её userdata-owner.
Primitive metatable сохраняется; byte count остаётся точным. Ошибка allocation
не оставляет dangling address index или orphan buffer.

Исправлен старый комментарий value.h: **сырой C pointer объекта не является
корнем GC**. Для хранения через collection нужен global reachability, pin или
GC root scope. Context-owned strings по текущему контракту остаются живыми до
context destruction; object collection не освобождает этот arena.
Точный учёт не означает traced string collection или завершение GC/stage 7.
Отдельная opt-in политика освобождения строк потребует полного string-root
аудита, включая native TextField pointers и borrowed C values.

Проверки: gc_string_pool — физические размеры counted NUL/WTF-8, exceptions,
stack strings и сохранение borrowed arena buffers после object GC;
gc_native_string_result — userdata poison при финализации для getter/method/
__index с сохранением ранее возвращённого значения;
gc_native_string_allocation — отказы new на каждой границе, index consistency,
GC и teardown balance. Остальные GC/UTF-16/serialization тесты проверяют
совместимость зарегистрированных buffers и rollback.

## English

v0.68 records exact payload lengths alongside context-owned string buffers.
Descriptor/address-index registration is atomic. Failed adoption retains caller
cleanup responsibility; decoding rollback erases ownership indexes before
freeing buffers, and context teardown frees registered allocations exactly once.

fvm_gc_string_stats reports buffer count, payload bytes and payload-plus-
terminator allocated bytes. It excludes container/allocator overhead, external
resource strings and process RSS. Stored byte counts preserve embedded NUL and
WTF-8 surrogate payloads. Query neither allocates nor invokes callbacks and
preserves caller output on errors.

Native accessor/method/index/value-metamethod string results become context-
owned snapshots before native receiver/capture roots unwind. Existing context
buffers are reused. Primitive metatables and counted bytes are preserved, and
allocation failure leaves no dangling ownership index.

The value.h lifetime comment now distinguishes object roots from context-owned
string storage: a returned object C pointer is not a GC root. Retained objects
need reachability, pins or root scopes. The existing string arena lasts until
context destruction; this change does not introduce traced string reclamation
or certify stage 7. Opt-in string collection still requires complete native
and borrowed-value root auditing, including TextField's native text pointer.

Tests verify binary/WTF-8 lengths, exception/stack accounting, borrowed arena
lifetime, owner-finalization independence, injected allocation rollback/index
consistency and teardown balance, alongside existing GC/UTF-16/serialization
regressions.

Проверено / Verified: full headless 246/246, AGG 262/262, Cairo 262/262, portable 237/237; targeted ASan/UBSan 33/33. Leak detection and stack ASan are disabled; actual Windows/macOS/Android execution and complete stage 7 acceptance remain open.

## Обновление v0.72 / v0.72 update

Описанная выше arena policy остаётся default. Opt-in traced collection добавлена
в v0.72; контракт корней и проверенные случаи: [GC_TRACED_STRINGS.md](GC_TRACED_STRINGS.md).

The historical arena lifetime remains the default. v0.72 adds opt-in tracing;
see GC_TRACED_STRINGS.md for rooting contracts, verification and remaining limits.
