/* Copyright (c) 2026 bhgv
 * EN: Portable scalar interface records shared by the Dis loader and Styx SDK.
 * RU: Переносимые записи интерфейса для загрузчика Dis и SDK Styx.
 */
#ifndef STYX_SCALAR_VERIFY_H
#define STYX_SCALAR_VERIFY_H
#include <stddef.h>
#include <stdint.h>
#define DVM_REF_SIGNATURE 0xadb273baU /* Compiled fn(ref Cell): (int,big). */
#define STYX_REF_REQUEST_TAG 81UL /* ADT snapshot argument. */
#define STYX_REF_CALLER_TAG 82UL /* ADT snapshot with caller context. */
#define STYX_REF_REPLY_TAG 83UL /* Return tuple followed by the updated ADT fields. */
#define DVM_TUPLE_SIGNATURE 0xbbfc530eU /* Compiled fn((int,big)): (int,big). */
#define STYX_TUPLE_REQUEST_TAG 78UL /* Tuple argument. */
#define STYX_TUPLE_REPLY_TAG 79UL /* Tuple result. */
#define STYX_TUPLE_CALLER_TAG 80UL /* Tuple argument with caller PID. */
#define DVM_SCALAR_BIG_SIGNATURE 0x17f525e4U /* Compiled fn(big): big signature. */
#define STYX_FUNCTION_BIG_TAG 76UL /* Signed big request on an opened function. */
#define STYX_FUNCTION_BIG_CALLER_TAG 77UL /* Signed big request with caller PID. */
#define DVM_SCALAR_INT_SIGNATURE 0xe67bf126U /* Compiled fn(int): int signature. */
#define STYX_FUNCTION_PROFILE_TAG 71UL /* Profile of a mounted function directory. */
#define STYX_FUNCTION_REQUEST_TAG 72UL /* Request on an opened function. */
#define STYX_FUNCTION_CALLER_TAG 74UL /* Request with current caller PID. */
#define STYX_FUNCTION_CALLER_PROFILE 75UL /* Export requires caller context. */
#define STYX_FUNCTION_REPLY_TAG 73UL /* Reply on the same open. */
#define STYX_MOUNTED_EXPORT_LIMIT 8U /* Maximum function files per module. */
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
/* EN/RU: Return the matching record bit in a complete, bounded mounted profile.
 * Вернуть бит совпавшей записи полного ограниченного профиля модуля. */
unsigned styxscalar_profile_match(const void *bytes, size_t size, const char *name, uint32_t signature, int *caller);
/* EN/RU: Identify a supported request tag by compiled signature, or zero.
 * Сопоставить поддерживаемый тег с сигнатурой либо вернуть ноль. */
uint32_t styxscalar_request_signature(unsigned long tag);
/* EN: Return the required reply tag or zero for an unknown request.
 * RU: Вернуть тег ответа либо ноль для неизвестного запроса. */
unsigned long styxfunction_reply_tag(unsigned long request);
#endif
