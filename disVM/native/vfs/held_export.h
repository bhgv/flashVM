/* Copyright (c) 2026 bhgv
 * EN: One-use publication of an exclusively owned native open file.
 * RU: Одноразовая публикация открытого файла с исключительным владением.
 */
#ifndef STYX_HELD_EXPORT_H
#define STYX_HELD_EXPORT_H
#include "provider.h"
#include "../../runtime/vfs/export.h"


/* EN/RU: Move source into a new owned dot provider on success. The caller
 * supplies a nonempty subset of its actual READ/WRITE authority, retires
 * all borrowers, and keeps source on failure. Metadata collection may block.
 * Передать source во владение новому dot provider при успехе. Вызывающий
 * задаёт непустое подмножество действительных прав READ/WRITE, завершает
 * заимствования и сохраняет source при отказе. Чтение metadata может блокировать. */
DVM_Provider *styxheldexport(DVM_VfsHandle *source, unsigned access);


/* EN/RU: Map an existing one-use grant at /.fd/number in a private view builder.
 * Caller retains view and grant; success retains a separate grant pin in the view.
 * Выдать существующий одноразовый файл по /.fd/number в частном view builder.
 * View и grant заимствованы; успех удерживает отдельную ссылку в view. */
int styxheldfdmap(DVM_View *view, unsigned number, DVM_Provider *grant, unsigned access);

#endif
