/* Copyright (c) 2026 bhgv
 * EN: Startup namespace ownership; initialization precedes guest threads.
 * RU: Владение startup namespace; инициализация до guest threads.
 */
#ifndef DVM_VFS_BOOT_H
#define DVM_VFS_BOOT_H
#include "namespace.h"


/* EN: Build initial group from host cwd and borrowed package directory, including
 * bundled virtual resources. Return one/zero; failure publishes nothing.
 * RU: Создать начальную группу из host cwd и package directory с bundled
 * virtual resources. Один/ноль; отказ без публикации. Только startup controller. */
int dvm_namespace_boot_initialize(const char *package_directory);


/* EN/RU: Borrow immutable startup group pointer; no foreign guest mutation rights.
 * Заимствовать startup group pointer; прав изменения чужого гостя не даёт. */
DVM_Namespace *dvm_namespace_boot_group(void);


/* EN/RU: Consume startup reference after all native owners join; NULL is harmless.
 * Снять startup reference после join всех native owners; NULL безопасен. */
void dvm_namespace_boot_shutdown(void);
#endif
