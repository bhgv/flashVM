# Сборка строк v0.72 / Traced strings

`fvm_gc_string_mode(ctx, FVM_GC_STRINGS_TRACED)` явно включает освобождение
недостижимых context string buffers. Default [FVM_GC_STRINGS_ARENA](../include/flashvm/gc.h#L27) сохраняет
старое время жизни до context destruction. [`fvm_gc_string_state`](../include/flashvm/gc.h#L37) читает политику.
Смена режима допускается только в owning VM thread вне collection/native calls;
включение не запускает GC. Отключение не восстанавливает уже освобождённые строки.
[FVM_Value](../include/flashvm/value.h#L26)/FVM_Config layout не изменены. Object reclaimed count по-прежнему
считает объекты; размеры строк отдельно показывает [fvm_gc_string_stats](../include/flashvm/gc.h#L132).

В traced mode сырой C pointer строки не является корнем. Хост должен хранить
значение в reachable property/index, GC root scope или зарегистрированном stack.
Object pin сохраняет строки его достижимого графа, но не отдельный string view.
Корни копируются: если хост меняет локальную переменную, ранее добавленный в scope
корень не меняется. Внешние буферы не принадлежат сборщику. Native userdata,
удерживающий context string pointer, должен предоставить явный value root либо
reachable managed capture; неописанная память userdata не сканируется.

Трассируются строки существующих object properties/indexes, primitive values
в API/AVM1 root frames, registers/locals, stack API и pending exceptions. Native
TextField имеет дополнительный implicit root для Field::text, не представленного
ordinary property. Foreign native string results сначала копируются в context
storage внутри native call barrier; GC сохраняет возвращённый результат только
при наличии последующего корня. Конкурентный доступ к одному context запрещён;
разные worker contexts имеют независимые arenas/policies.

План collection до callbacks выделяет immutable range index и mark bytes для
старых buffers. Flat-address starts сортируются; binary lookup сохраняет buffer
также для interior view или пустой строки на его terminator. Сами чужие pointers
не разыменовываются. Initial и resurrection marking используют общие object/value
roots. Перед remark string marks сбрасываются. Buffers, созданные финализатором,
сохраняются до следующего collection, как newborn managed objects. Финализатор
может resurrect объект со старыми строками или публиковать новые строки.

После native release callbacks без allocation повторно проверяются string values
зарегистрированных stacks/frames и surviving objects: release callback может
поместить borrowed view в существующий stack после resurrection remark.

Ни один string buffer не освобождается до завершения finalizers, native resource
release и object sweep. Затем unmarked старые buffers удаляются, descriptor vector
compacts, существующие address-map indexes обновляются без allocation/callback.
Ошибка выделения плана или marking не освобождает strings и снимает collecting
flag; возможные side effects уже вызванных финализаторов не откатываются.
При traced policy успешные string allocations учитываются для automatic GC
scheduling; actual collection остаётся на существующих safe points.

Проверки: gc_traced_strings — NUL/interior roots, pending exceptions,
resurrection, newborn grace и обе политики; gc_traced_avm1 — 1000 concatenations
при maximal collection frequency; gc_traced_string_allocation — every-new failure,
indexes и teardown balance; gc_traced_release_roots — borrowed view, помещённый
release callback в stack после remark; gc_native_string_result — userdata poison в обеих
политиках; gc_traced_string_transport — queue/file/TCP cyclic graphs и четыре
workers по 1000 строк; gc_traced_textfield — implicit text roots, edited old views,
font paths и восстановление через queue. TextField test требует FONTS и установленную
text library получателя; TCP — OS_PORTS.

Это opt-in точечная приёмка, а не завершение всего GC/отладчика. Полный аудит
всех native extension contracts, реальные SWF, длительные mixed-resource stress,
межпроцессные fault cases и distributed deadlock graph остаются открытыми.

## English

Opt in with [fvm_gc_string_mode](../include/flashvm/gc.h#L33)(ctx, [FVM_GC_STRINGS_TRACED](../include/flashvm/gc.h#L28)). The default ARENA
policy preserves context-long borrowed string lifetime. Policy query does not
allocate; policy changes are rejected during collection/native callbacks and
do not trigger collection or restore previously reclaimed buffers. Public
[FVM_Value](../include/flashvm/value.h#L26)/FVM_Config layouts are unchanged; object reclaimed counts still count
objects, and string byte accounting is separate.

Raw borrowed string pointers are not roots in traced mode. Keep values in
reachable properties/indexes, registered stacks or GC root scopes. Object pins
retain reachable graph values, not unrelated string views. Root scopes hold
copies, not live bindings to host variables. External buffers remain external;
opaque native userdata must explicitly root any retained context string values.
Concurrent access to one context is unsupported; independent worker contexts
have separate string ownership and policies.

Existing object/value root traversal now marks strings, including API/AVM1
frames, registers/locals, stack API and pending exceptions. Native TextField text
has an implicit reachable-owner root. Native external results are copied inside
the call barrier; returned values must subsequently be rooted by the caller.
A pre-finalization sorted range index also recognizes interior and terminator
views without dereferencing external pointers. Resurrection remark resets marks;
finalizer-created buffers survive this collection and may be reclaimed next time.

An allocation-free late root pass retains old views newly published into existing
stacks/frames or surviving objects by native resource-release callbacks.
Only after finalizers, resource release and object sweep are complete does the
collector release unmarked original buffers and compact ownership indexes without
allocation/callbacks. Preparation/marking OOM leaves string allocations intact
and resets collection state; already-executed finalizer side effects are not
rolled back. Traced string allocations contribute to automatic scheduling while
collection remains restricted to existing safe points.

Regressions cover both policies, embedded NUL/interior roots, exceptions,
resurrection/newborn retention, repeated AVM1 safe points, injected allocation
failures/index consistency, native owner poison, cyclic queue/file/TCP graph
transport, four independent worker workloads and native TextField/editing/font
roots. TextField restoration uses a receiver with its text library installed;
font and TCP cases require their respective enabled features. Full native
extension root auditing, real SWF acceptance, sustained mixed-resource/process
fault tests and distributed debugger acceptance remain open.

Проверки v0.72 / verification: full headless 253/253, AGG 269/269, Cairo
269/269, portable 244/244; targeted ASan/UBSan 44/44. TextField/font cases
run in AGG/Cairo; fonts-disabled sanitizer excludes live font cases. ASan
heap/global instrumentation and UBSan active; leak detection and stack ASan
disabled. No Windows/macOS/Android runtime certification. Initial failures
and final passing logs are retained outside Git. Comment/GC-tag audits pass.

## Обновление v0.73 / v0.73 update

Отдельный allocation-free late string pass v0.72 заменён общим late object/value
graph traversal после native release callbacks. Он сохраняет также object roots
и новые edges у ранее reachable объектов; контракт и OOM: GC_RELEASE_GRAPH.md.

The v0.72 string-only late pass is superseded by a full late graph traversal
covering object roots and edges added to previous survivors. See GC_RELEASE_GRAPH.md.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:55](../include/flashvm/flashvm.h#L55) |
| `fvm_gc_string_mode` | function | [include/flashvm/gc.h:33](../include/flashvm/gc.h#L33) |
| `fvm_gc_string_state` | function | [include/flashvm/gc.h:37](../include/flashvm/gc.h#L37) |
| `fvm_gc_string_stats` | function | [include/flashvm/gc.h:132](../include/flashvm/gc.h#L132) |
| `FVM_GC_STRINGS_ARENA` | macro / define | [include/flashvm/gc.h:27](../include/flashvm/gc.h#L27) |
| `FVM_GC_STRINGS_TRACED` | macro / define | [include/flashvm/gc.h:28](../include/flashvm/gc.h#L28) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
