# Loaded generic splice mutation / Общий splice в SWF — v0.154

Общий loaded splice переносит значения по own presence/raw data, без getter,
prototype lookup, setter и watcher. Source удаляется после разрешённого raw
store, кроме перемещения в ту же позицию. Hole удаляет destination. Items
записываются без предварительного удаления descriptor. Собственные flags
назначения сохраняются; DONT_DELETE источника может оставить его descriptor.

Длина borrowed Object записывается напрямую. В отличие от native Array,
хвост borrowed Object не удаляется только из-за новой длины: удаление всего
диапазона оставляет прежние индексные свойства за length=0. Native generic
resize удаляет tail, соблюдая DONT_DELETE, но protected tail не удерживает
length; readonly length сохраняет данные длины после попыток удаления.

array_legacy_splice_mutation проверяет SWF6..8, переносы в обе стороны, virtual
raw data/readonly/nondelete, отсутствие callbacks/watch, inherited holes,
borrowed tail, native generic protected tail и counted строки после traced GC.
Сохраняются generic лимит 65536 и signed new-length INT32_MAX; ошибки после
мутации не откатывают её. Быстрый native splice сохраняет старый путь и его
OOM-проверки; его watch/flag поведение ещё открыто. Writable native C accessor data,
case/version и все стадии остаются открытыми. Getter-only C accessor игнорирует raw запись, сохраняя embedding контракт.

## English

Loaded generic splice moves by own presence/raw data without getters, prototype
lookup, setters or watchers. It deletes the source after successful raw storage
unless source equals destination; holes delete destinations. Inserted items do
not first delete descriptors. Destination flags remain attached to properties;
nondeletable sources may retain descriptors. Borrowed length is stored directly
without native tail deletion, so deleting an entire borrowed range retains old
indices beyond zero length. Native generic resize deletes tails respecting
nondelete, but retained tails do not clamp length; readonly length data persists.
SWF6..8 tests cover both directions, virtual flags, no callbacks/watch, inherited
holes, borrowed/native protected tails and counted strings after traced GC.
Generic length above 65536/new signed length above INT32_MAX stays UNSUPPORTED.
Partial mutations are not rolled back. The existing native fast path and its OOM
tests remain unchanged; its watch/flag rules, native accessor data, case/version
and every roadmap stage remain open. Getter-only C accessor writes are ignored, preserving the embedding contract.
