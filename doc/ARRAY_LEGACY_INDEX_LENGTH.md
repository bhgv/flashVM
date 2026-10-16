# Indexed length ordering / Порядок indexed assignment — v0.170

В загруженном native Array запись индекса сначала пытается увеличить length,
после watcher и перед проверкой flags данных/вызовом setter. READ_ONLY length
сохраняет длину, но не запрещает own data/virtual setter за её пределами.
Readonly virtual data по-прежнему защищены; function setter выполняется.
Shared note_index также учитывает readonly длину. Незагруженный guard сохранён.

Decoder graph больше не требует index < length: законные хвосты после readonly
length или raw сокращения допускаются. Reserved native индекс 0xffffffff
по-прежнему отклоняется; format/ABI версии не меняются.
array_legacy_index_length проверяет SWF4..8 и незагруженный режим, вне-length
объект со строкой a\0b после traced GC/queue transfer, readonly virtual setter,
рост до flags check, защищённые stored data и invalid reserved index wire с
правильной CRC без выделения destination objects.

Виртуальная native length, signed/wrapping index semantics, версия атрибутов,
ordinary length truncation/DONT_DELETE и все стадии остаются открытыми.
Ошибка памяти после роста может сохранить увеличенную длину без новой записи,
как последовательность set_local эталона; loaded атомарность не заявлена.

## English

Loaded native Array indexed assignment attempts length growth after watching and
before data flags/setters. Readonly length preserves the reported length while
allowing own data and virtual setters beyond it. Readonly virtual data remains
protected. Shared note_index honors readonly length; unloaded guarding is retained.

Graph decoding permits legal indexed tails outside stored Array length while
still rejecting reserved native index 0xffffffff. Wire format and ABI are unchanged.
Tests cover loaded SWF4..8/unloaded mode, an outside child with counted strings
through traced GC/channel transfer, readonly setter dispatch, growth before flags,
protected stored data and CRC-valid malformed reserved-index rejection without
allocating destination objects. Virtual length, signed/wrapping indices, version
masks, ordinary truncation/nondelete rules and all stages remain open. Allocation
failure after growth may leave the longer length without a completed write;
loaded atomicity is not claimed. Reference: pinned ScriptObject.set_local.

## v0.433 clarification / Уточнение

The growth-before-setter rule above applies to own writes. Inherited virtual
setters dispatch before Array.set_local and do not grow length; success/throw,
watcher errors, GC and actual native paths are checked in WATCH_THROW.md.
Правило роста до setter относится к собственным записям. Наследуемый virtual
setter обрабатывает запись до Array.set_local без роста; подробности и проверки
в WATCH_THROW.ru.md. Полная Array-совместимость и все стадии остаются открытыми.
