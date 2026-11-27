/* Copyright (c) 2026 bhgv
 * [GC:] EN: Private native resource lookup by its owner and release callback.
 * RU: Внутренний поиск нативного ресурса по владельцу и функции освобождения.
 */
#ifndef FVM_USERDATA_INTERNAL_H
#define FVM_USERDATA_INTERNAL_H
#include "flashvm/userdata.h"

#ifdef __cplusplus
extern "C" {
#endif


/* [GC:] EN: Borrow the payload of object in context only if release identifies its type.
 * Return NULL on a mismatch or during resource destruction. The caller roots object.
 * RU: Заимствовать данные object в context, только если release определяет его тип.
 * Вернуть NULL при несовпадении или освобождении ресурсов. Вызывающий удерживает object. */
void *fvm_userdata_get_bound(FVM_Context *context, FVM_Object *object, FVM_UserdataRelease release);

#ifdef __cplusplus
}
#endif
#endif
