/* Copyright (c) 2026 bhgv
 * EN: Write a type trailer from the compiler's resolved exported declarations.
 * RU: Записать секцию типов из разрешённых деклараций экспортов компилятора.
 */
#ifndef LIMBO_STYX_EXPORT_H
#define LIMBO_STYX_EXPORT_H

struct Decl;


/* EN: Append the type table for NC64, using resolved module exports and ldts.
 * RU: Добавить таблицу типов экспортов module и импортов ldts для NC64. */
void limbo_styx_export(struct Decl *module, struct Decl *ldts);

#endif
