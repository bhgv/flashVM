/* Copyright (c) 2026 bhgv
 * EN: A portable offer produced from a linked export on the owner VM.
 * RU: Переносимое предложение из связанного экспорта VM владельца.
 */
#ifndef STYX_LOAD_PROFILE_H
#define STYX_LOAD_PROFILE_H

#include <stddef.h>
#include <stdint.h>


/* EN: Select a supported unscoped call profile for the compiled signature.
 * RU: Выбрать профиль вызова без PID по скомпилированной сигнатуре. */
unsigned long styxloadedrequest(uint32_t signature);


/* EN: Encode the linked name/signature into borrowed bytes up to capacity.
 * RU: Закодировать связанное имя и сигнатуру в bytes объёмом capacity. */
int styxloadedoffer(const char *name, uint32_t signature, void *bytes, size_t capacity, size_t *used);


/* EN: Verify a complete export offer for name, and return its request tag.
 * RU: Проверить полное предложение для name и вернуть тег запроса. */
unsigned long styxloadedaccept(const char *name, const void *bytes, size_t size);

#endif
