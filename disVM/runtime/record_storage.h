/* Copyright (c) 2026 bhgv
 * EN: Private record storage operations for both admission paths.
 * RU: Операции над приватной памятью записей для обоих путей допуска.
 */
#ifndef DVM_RECORD_STORAGE_H
#define DVM_RECORD_STORAGE_H

/* EN: Validate address geometry or a borrowed live type descriptor.
 * RU: Проверить диапазон адреса или заимствованный живой дескриптор типа. */
int dvm_record_span(const void *address, size_t size);
int dvm_record_valid_type(Type *type);

/* EN: Initialize already allocated private payload with a validated map.
 * RU: Инициализировать выделенную приватную память по проверенной карте. */
void dvm_record_initialize_payload(Type *type, int clear, uchar *data, size_t bytes, WORD count);

#endif
