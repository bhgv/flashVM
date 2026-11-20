/* Copyright (c) 2026 bhgv
 * EN: One-shot Unibus receive file with a bounded record and independent read cursors.
 * RU: Одноразовый файл приёма Unibus с ограниченной записью и отдельными позициями чтения.
 */
#ifndef STYX_UNIBUS_INBOX_H
#define STYX_UNIBUS_INBOX_H
#include "provider.h"


/* EN/RU: Return an owned native provider for one complete S-record with the specified tag.
 * Map separate READ and WRITE capabilities before sharing it.
 * Вернуть провайдер одной полной S-записи заданного тега. Перед публикацией
 * выдавать права чтения и записи через отдельные отображения. */
DVM_Provider *styxunibusinbox(unsigned long tag);

#endif
