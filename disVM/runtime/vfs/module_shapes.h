/* Copyright (c) 2026 bhgv
 * EN: Owned compiler type metadata on a loaded Dis module.
 * RU: Метаданные типов компилятора у загруженного модуля Dis.
 */
#ifndef DVM_MODULE_SHAPES_H
#define DVM_MODULE_SHAPES_H

#include "../../native/terminal/load_profile.h"

struct Module;


/* EN: Attach a checked table after sourcepath's legacy NUL, if present.
 * start/end are borrowed parse bounds. Refusal leaves module unchanged.
 * RU: Присоединить проверенную таблицу после NUL пути исходника, если она есть.
 * start/end задают заимствованные границы. При отказе модуль сохраняется. */
int dvm_module_read_shapes(struct Module *module, const unsigned char *start, const unsigned char *end);


/* EN: Copy one compiled schema by validated export ordinal, or return zero.
 * RU: Скопировать схему по проверенному порядковому номеру экспорта либо вернуть ноль. */
int dvm_module_export_shape(const struct Module *module, size_t index, StyxCallShape *shape);

#endif
