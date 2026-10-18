# Подготовка финализаторов v0.75 / Finalizer preparation

Раньше finalized=true устанавливался до подготовки ActionScript-вызова: OOM в
roots, function metadata, activation/arguments, lexical scope или Action parser
мог лишить объект __gc без входа в пользовательский код. Также исключение при
lexical-scope registration после script_depth++ оставляло неверную глубину.

Resolved finalizer теперь armed в context pending_finalizer на время dispatch.
Первый native callback guard либо завершение подготовки AVM1 body перед Action
dispatch фиксирует finalized и очищает arm. Scope восстанавливает предыдущий
pending pointer на всех выходах. Функция без callable handler фиксируется без
allocation; terminal non-OOM preparation errors завершают попытку.

OOM **до входа в пользовательский код** не фиксирует finalized. GC прерывает
проход до native resource release/sweep, оставляет objects/strings выделенными,
снимает collecting flag и позволяет retry. Частично созданные activation objects
остаются в managed heap до последующей collection. Начавшийся native callback/
AVM1 body не повторяется, даже если возвращает OOM, error или exception: уже
выполненные side effects не откатываются. Если подготовка вызывает пользовательский
native accessor, entry guard считает этот callback началом пользовательского
кода; повторение таких side effects не допускается.

Script-depth RAII восстанавливает прежнюю глубину при любых preparation errors,
включая lexical scope push. Новые поля внутренние: public value/config ABI не
меняется. Во время обычных вызовов pending_finalizer=NULL, entry marker не делает
ничего. Одновременное использование одного context несколькими OS threads не
поддерживается. Native callback root/barrier и resurrection contracts сохраняются.

gc_script_finalizer_preparation перебирает каждый new failure для named и
function-object finalizers через direct dispatch и полный GC. Проверяются zero-
allocation preparation failure, retry, once-only после body entry, оба вида
failure, script depth, frames/scopes, pending/collecting/native-depth cleanup,
последующая collection и allocation balance. Реальные SWF и полная приёмка
GC/debugger остаются открытыми; teardown void API по-прежнему best-effort для
скриптов, требующих новой памяти.

## English

Finalizer state used to be committed before script preparation, losing __gc on
root/metadata/activation/arguments/scope/parser OOM. Script depth could also remain
incremented when lexical-scope registration threw.

A resolved invocation is now armed until the first user native callback guard or
AVM1 body dispatch begins. Entry commits once-only finalization; scoped state
restores the previous arm on every exit. No-handler and terminal non-OOM setup
errors consume the attempt. An accessor callback reached during preparation also
counts as user-code entry, preventing duplicate observable callback effects.

Pre-entry OOM leaves finalization retryable and aborts GC before resource release
or sweep, preserving managed objects/strings and resetting collection state.
Partial activation objects remain managed until a later collection. Entered bodies
are never replayed after partial effects, including OOM/error/exception. Script
call-depth RAII restores the exact prior depth after all preparation failures.
Public value/config ABI is unchanged and ordinary calls have no armed finalizer.

Failure injection covers named/function-object finalizers through direct dispatch
and collection, zero-allocation setup failure, pre-entry retry, post-entry once-only
behavior, depth/frame/scope/arm/barrier cleanup, subsequent GC and allocation
balance. Real SWF/full debugger acceptance remains open. Void context teardown is
still best-effort for script finalizers needing new allocation.

Проверки v0.75 / verification: full headless 257/257; targeted GC/AVM1/
Function/metatable/resource/debugger/text tests 69/69 in AGG, Cairo, portable
and ASan/UBSan. Fonts-enabled checks run in AGG/Cairo; portable excludes TCP.
Heap/global ASan and UBSan active; leak detection/stack ASan disabled. Linux
profiles only, complete GC/debugger acceptance remains open.
