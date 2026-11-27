/* Copyright (c) 2026 bhgv
 * EN: VFS-side constructor; Inferno translation units include only file.h.
 * RU: Конструктор стороны VFS; Inferno translation units включают только file.h.
 */
#ifndef STYX_FILE_PROVIDER_H
#define STYX_FILE_PROVIDER_H
#include "file.h"
#include "provider.h"
/* EN/RU: Retain a borrowed prepared provider's root file; owned backing or NULL.
 * No raw host-path lookup, guest execution or rights widening. Provider remains borrowed.
 * Удержать корневой файл prepared provider; свой backing/NULL. Без host path и расширения прав. */
StyxBacking *styxbacking(DVM_Provider *provider);
/* EN/RU: Retain a prepared directory root with identity-bound descendant nodes.
 * Удержать prepared-каталог с привязкой дочерних node к identity; свой wrapper/NULL. */
StyxBacking *styxtree(DVM_Provider *provider);


/* EN: Inspect a borrowed exclusive cursor for cancellable byte read and any required seek.
 * RU: Проверить, допускает ли заимствованный курсор отмену чтения байтов и нужного seek. */
int styxfilecancellableread(StyxOpen *opened, unsigned long long offset);


/* EN: Check whether the borrowed cursor can cancel a byte write and any required seek.
 * RU: Проверить, допускает ли заимствованный курсор отмену записи и нужного seek. */
int styxfilecancellablewrite(StyxOpen *opened, unsigned long long offset);

#endif
