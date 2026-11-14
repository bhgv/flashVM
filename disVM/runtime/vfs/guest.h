/* Copyright (c) 2026 bhgv
 * EN: Guest namespace adapters; include interp.h before this header.
 * RU: Guest-адаптеры namespace; подключать после interp.h.
 */
#ifndef DVM_VFS_GUEST_H
#define DVM_VFS_GUEST_H
#include "namespace.h"


/* EN: Copy current guest's resolved native path into output, or retain raw
 * startup path without a current guest. Return one/zero with guest error.
 * ACTIVE caller owns its Prog; output has DVM_NS_PATH_LIMIT bytes and contains
 * no managed pointers.
 * RU: Скопировать native path текущего гостя в output; без гостя оставить
 * startup path. Один/ноль с guest error. ACTIVE caller владеет своим Prog;
 * output вмещает DVM_NS_PATH_LIMIT bytes, без managed pointers. */
int dvm_namespace_host_path(const char *path, char *output);
#endif
