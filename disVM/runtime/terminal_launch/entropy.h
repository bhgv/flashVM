/* Copyright (c) 2026 bhgv
 * EN: Shared host entropy for terminal launch and cell capabilities.
 * RU: Общая системная энтропия для запуска и полномочий ячеек.
 */
#ifndef DVM_TERMINAL_ENTROPY_H
#define DVM_TERMINAL_ENTROPY_H
#include <stddef.h>

/* EN: Fill size writable bytes with OS entropy. Return one on completion.
 * RU: Заполнить size доступных для записи байтов системной энтропией. Единица при успехе. */
int dvm_terminal_entropy(unsigned char *bytes, size_t size);
#endif
