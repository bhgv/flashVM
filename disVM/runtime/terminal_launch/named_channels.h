/* Copyright (c) 2026 bhgv
 * EN: Mount the session's named-channel service in the reverse live namespace.
 * RU: Смонтировать сервис имён каналов в живой встречной VFS.
 */
#ifndef DVM_NAMED_CHANNELS_H
#define DVM_NAMED_CHANNELS_H
#include "../vfs/resource.h"

/* EN: Install a private shared service for the lifetime of the paired guest group.
 * RU: Установить закрытый сервис на время жизни группы парной программы. */
int dvm_named_channels_install(DVM_Namespace *group);
#endif
