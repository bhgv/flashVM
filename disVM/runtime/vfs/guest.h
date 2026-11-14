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


/* EN: Copy current guest/startup group's resolved native path into output;
 * retain raw path only before startup group exists. One/zero with guest error.
 * ACTIVE caller owns its Prog; output has DVM_NS_PATH_LIMIT bytes and contains
 * no managed pointers.
 * RU: Скопировать native path guest/startup группы в output; raw path допустим
 * только до создания startup group. Один/ноль с guest error; caller владеет Prog;
 * output вмещает DVM_NS_PATH_LIMIT bytes, без managed pointers. */
int dvm_namespace_host_path(const char *path, char *output);
#endif
