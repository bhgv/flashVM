/* Copyright (c) 2026 bhgv
 * EN: Namespace lifetime checks for the native TCP mount fixture.
 * RU: Проверки времени жизни namespace для штатного TCP-маунта.
 */
#ifndef DISVM_TEST_MOUNT_DETACHED_H
#define DISVM_TEST_MOUNT_DETACHED_H
#include "mount.h"


/* EN: Borrow provider and return an open fid after discarding its namespace.
 * RU: Заимствовать provider и вернуть открытый fid после удаления namespace. */
DVM_VfsHandle styxtest_detached_open(DVM_Provider *provider);


/* EN: Consume provider and check union lookup and rewind after TCP loss.
 * RU: Принять provider и проверить поиск и перемотку union после обрыва TCP. */
void styxtest_union_loss(DVM_Provider *provider);

#endif
