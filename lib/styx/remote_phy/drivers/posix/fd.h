/* Copyright (c) 2026 bhgv
 * EN: Explicit adoption of an already granted nonblocking POSIX carrier descriptor.
 * RU: Явное принятие уже выданного nonblocking POSIX carrier descriptor.
 */
#ifndef REMOTE_PHY_POSIX_FD_H
#define REMOTE_PHY_POSIX_FD_H
#include "../../phy.h"
#ifdef __cplusplus
extern "C"
{
#endif


/* EN/RU: Consume fd on every outcome; owned physical adapter/NULL, no open/dial/authentication.
 * Caller explicitly asserts carrier capabilities; fd alone does not prove reliable framing.
 * Supports O_RDWR sockets and byte devices; configures NONBLOCK/CLOEXEC, preserves other flags.
 * Host configures serial/bus details before adoption; wake uses a private native pipe.
 * Принять fd всегда; свой adapter/NULL, без open/dial/auth. Caller явно задаёт capabilities;
 * O_RDWR socket/byte device, NONBLOCK/CLOEXEC; serial/bus configuration делает host до adoption. */


/* [standalone-contract:phyposixfd]
 * EN: phyposixfd: posix/fd implementation and native state.
 * RU: phyposixfd: реализация posix/fd и нативное состояние.
 * descriptor (int descriptor): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * capabilities (unsigned capabilities): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: RemotePhy *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: RemotePhy *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
RemotePhy *phyposixfd(int fd, unsigned capabilities);
#ifdef __cplusplus
}
#endif
#endif
