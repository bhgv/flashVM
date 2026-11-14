/* Copyright (c) 2026 bhgv
 * EN: Guest namespace adapters; include interp.h before this header.
 * RU: Guest-адаптеры namespace; подключать после interp.h.
 */
#ifndef DVM_VFS_GUEST_H
#define DVM_VFS_GUEST_H
#include "namespace.h"


/* EN: Borrow current permanent owner's group, or the startup group before
 * guest entry. ACTIVE owner/startup controller only; no foreign access/ref.
 * RU: Заимствовать группу своего owner либо startup group до guest entry.
 * Только ACTIVE owner/startup controller; без foreign access/новой ссылки. */
DVM_Namespace *dvm_namespace_current_group(void);


#endif
