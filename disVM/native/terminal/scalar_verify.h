/* Copyright (c) 2026 bhgv
 * EN: Portable scalar interface records shared by the Dis loader and Styx SDK.
 * RU: Переносимые записи интерфейса для загрузчика Dis и SDK Styx.
 */
#ifndef STYX_SCALAR_VERIFY_H
#define STYX_SCALAR_VERIFY_H
#include <stddef.h>
#include <stdint.h>
#define INTERFACE_KIND 4U /* Distinct from call, value and error. */
#define INTERFACE_HEADER 5U /* One kind byte and a network-order signature. */
#define INTERFACE_NAME_LIMIT 48U /* Name bytes excluding NUL. */
#define STYX_SCALAR_INTERFACE_CAPACITY 64U /* Existing one-shot inbox capacity. */

/* EN/RU: Measure the bounded name, returning zero on failure.
 * Измерить имя в пределах лимита, при ошибке вернуть ноль. */
size_t styxscalar_name_length(const char *name);

/* EN/RU: Write the fixed header into borrowed storage of INTERFACE_HEADER bytes.
 * Записать заголовок в буфер из INTERFACE_HEADER байт. */
void styxscalar_interface_header(unsigned char *data, uint32_t signature);

/* EN/RU: Match one complete frame with borrowed import and tag.
 * Сверить полную запись с заимствованным импортом и тегом. */
int styxscalaraccept(unsigned long tag, const void *bytes, size_t size,
    const char *name, uint32_t signature);
#endif
