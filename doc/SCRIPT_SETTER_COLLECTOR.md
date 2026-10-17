# Script setter collection / GC в скриптовом setter

v0.253 extends trusted addProperty bridge entry to setters. The bridge omits only
its own native barrier; the existing [FVM_SetterScope](../src/core/setter_scope.hpp#L6) retains receiver, lookup
owner and snapshotted descriptor plus an immutable key. Script invocation retains
the copied incoming argument. Arbitrary native setters and enclosing host-native
calls keep their collector barriers; they are never suspended.

Prepared native script setters may explicitly collect, throw, delete their own
descriptor or detach the receiver's prototype. Stored-data replacement, inherited
setter behavior and readonly rules are unchanged. The descriptor bridge still
re-resolves replacement metadata after the callback; its original capture stays
rooted through self-removal. Setters do not become globally reentrant native APIs.

Tests cover numeric/object/borrowed-text inputs in traced-string mode, capture
liveness, exceptions, self-removal with own-data publication, inherited setter
prototype detachment, ordinary C setters and outer C callback barriers. Allocation
injection checks restoration of root chain, script/native depths and setter owner/
key metadata after every failure, then successful retry. Passing subsets do not
close full GC/debugger or platform/JIT acceptance.

Известный мост скриптового setter теперь допускает GC после установки корней.
Обычные C-setters и внешний нативный вызов сохраняют запрет. Проверяются текст,
объектные аргументы, capture, исключение, удаление дескриптора и разрыв связи
с прототипом. Отказы памяти не оставляют изменённых глубин вызова, цепочек корней
или метаданных setter. Полная приёмка стадий 7–9 остаётся открытой.

Evidence: 464 tests pass per Linux AGG/Cairo profile after recovering three AGG
execute-bit launch failures. Seven focused heap/global ASan+UBSan checks pass;
stack/leak instrumentation remains unavailable. All 25 original SWF<=8 probes
retain exact output under their recorded timeline policies in each profile.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_SetterScope` | class | [src/core/setter_scope.hpp:6](../src/core/setter_scope.hpp#L6) |
<!-- source-links:end -->
