# Explicit native clip detachment / Явное отсоединение клипа — v0.297

The C89 execution API now exposes
`fvm_exec_detach_clip(FVM_Context *context, FVM_Object *clip)`.
The operation accepts a native MovieClip instance from the supplied context.
Null operands, foreign objects, ordinary objects and the builtin class prototype
return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Destroying/releasing contexts are rejected.

This is the existing forced scene-removal policy: it accepts authored negative
and dynamic nonnegative depths, stops the detached instance and clears its native
parent/slot/current-name binding transactionally. It does not look up or call a
script property named removeMovieClip. Other aliases remain references to the
same detached wrapper. Repeated detachment is allowed. A parentless root is
stopped, matching the existing forced native operation.

[GC:] The service retains its explicit operand while preparing ownership maps.
Allocation failure restores the caller's root chain and leaves the parent,
child slot, name binding and playback state unchanged. Map publication uses
nonallocating swaps; no script callback or collection occurs during publication.
The caller retains the wrapper if it must survive subsequent collections.

The signature contains no Action opcode, bytecode, interpreter stack or program
counter. Generated bodies and C embedders can use the same operation once they
have selected and validated the applicable policy. No new resource identity,
transport schema, activation target or public ABI layout is introduced.

This does not implement Action RemoveSprite: historical dynamic-depth gating,
removed-descendant state, unload-event ordering, path resolution and activation
target fallback remain open. No opcode reference is added to the dispatch audit.
It is unsuitable as an unconditional replacement for historical script removal.

The C89 execution_detach_clip fixture covers host and loaded SWF4–8, positive and
negative depths, repeated calls, invalid ownership/type and script-method override
bypass. movieclip_remove_allocation now injects failures through this public entry,
checking native links and the root chain before GC between failed attempts.

Новый C89-сервис получает контекст и явный нативный клип. Он не читает байткод,
стек интерпретатора и заменяемый скриптовый метод. Принудительное отсоединение
поддерживает отрицательные глубины, повторный вызов и транзакционное обновление
связей; отказ памяти сохраняет исходный граф и цепочку корней GC. Неверный тип,
чужой объект и прототип класса отвергаются.

Это строительный блок для C-кода и будущего JIT. Историческая семантика
RemoveSprite, события выгрузки и восстановление текущей цели ещё не реализованы.
Ни один этап роудмапа этим изменением не закрывается.

Validation: Linux AGG and Cairo each passed all 581 tests. Nineteen AGG and nine
Cairo executables required a launch-permission repair and passed on retry.
Both affected heap/global ASan+UBSan fixtures passed; stack and leak coverage are
excluded. The first new C fixture run failed during setup because its second
context lacked standard installation; setup was corrected before successful
validation. All 26 accepted original SWFs matched the pinned oracle in each
renderer profile. Dispatch coverage remains 94 references out of the pinned
100-entry catalog, with six absent; this bounded audit is not semantic completion.

## Shared operation body — v0.298

The complete transactional detachment body now lives in the single-operation
movieclip_remove.cpp file. The stable removeMovieClip native callback and the C
execution service both delegate to [fvm_movieclip_scene_remove](../src/standard/movieclip_remove.cpp#L7). Native callback
serialization identifiers and the graph schema remain unchanged.

movieclip_storage.hpp contains the shared raw field/state/depth helpers used by
the main MovieClip implementation and the extracted operation. This keeps slot
validation, legacy bias handling and counted name cleanup consistent without
exposing interpreter internals or duplicating the transaction. Helpers do not
perform script property dispatch; callers retain the previous rooting/barrier
responsibilities. No historical removal policy or new Action is introduced.

Тело транзакционного отсоединения вынесено в отдельный movieclip_remove.cpp.
Нативный метод и C-сервис используют одну операцию. Общие проверки приватного
состояния и глубин вынесены в movieclip_storage.hpp; правила старых графов и
идентификаторы сериализации нативных методов сохранены. Это декомпозиция без
изменения заявленной семантики.

Validation for v0.298: Linux AGG and Cairo each passed 581 tests, with three AGG
and four Cairo launch-permission failures repaired and passed on retry. The four
affected heap/global ASan+UBSan fixtures passed (no stack/leak coverage). All 26
accepted original SWFs matched the oracle in both profiles. Native graph transfer,
legacy depth migration, named bindings and injected allocation failures are
included in regression coverage; full removal/event semantics remain open.

Follow-up v0.315: physical detachment remains unchanged, while loaded AVM1 reads
retain a separate original parent edge for _parent/_root/path provenance. Unloaded
host reads stay parentless. See MOVIECLIP_REMOVED_ANCESTRY.md.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:239](../src/core/runtime.hpp#L239) |
| `fvm_exec_detach_clip` | function | [include/flashvm/execution.h:237](../include/flashvm/execution.h#L237) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_scene_remove` | function | [src/standard/movieclip_remove.cpp:7](../src/standard/movieclip_remove.cpp#L7) |
| `FVM_Object` | struct | [src/core/runtime.hpp:119](../src/core/runtime.hpp#L119) |
<!-- source-links:end -->
