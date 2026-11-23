/* Copyright (c) 2026 bhgv
 * EN: Native callbacks for a live signed 32-bit Styx variable.
 * RU: Нативные обработчики живой 32-битной знаковой переменной Styx.
 */
#ifndef STYX_VARIABLE_FILE_H
#define STYX_VARIABLE_FILE_H
#include <stddef.h>
#include <stdint.h>

#define STYX_VARIABLE_INT_SIZE 4U /* Exact bytes per signed integer. */

typedef struct StyxIntVariable
{
    void *state; /* Borrowed owner context. */
    int (*load)(void *, int32_t *); /* Copy the current value; return 1 on success. */
    int (*store)(void *, int32_t); /* Apply one value; return 1 on success. */
} StyxIntVariable;


/* EN/RU: Read an entire value at offset zero or EOF at offset four or later.
 * Прочитать целое значение с нуля либо EOF со смещения четыре и далее. */
int styxintvariableread(const StyxIntVariable *variable, uint64_t offset, void *bytes, size_t *size);


/* EN/RU: Validate one entire update before calling the owner once.
 * Проверить полное изменение до единственного вызова владельца. */
int styxintvariablewrite(const StyxIntVariable *variable, uint64_t offset, const void *bytes, size_t size);

#endif
