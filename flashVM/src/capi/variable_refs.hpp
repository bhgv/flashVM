/* Copyright (c) 2026 bhgv
 * EN: Local reference operands and transparent AS bindings.
 * RU: Локальные ссылочные операнды и прозрачные привязки AS.
 */
#ifndef FVM_VARIABLE_REFS_HPP
#define FVM_VARIABLE_REFS_HPP
#include "flashvm/value.h"


/* EN/RU: Is this a private reference operand? / Это внутренний ссылочный операнд? */
bool fvm_is_variable_ref(const FVM_Value *);


/* EN/RU: Bind a new name to a reference operand. / Привязать новое имя к ссылке. */
FVM_Result fvm_bind_variable_ref(FVM_Context *, FVM_Object *, const char *, size_t, const FVM_Value *);


/* EN: If ref targets a raw numeric index, return a borrowed pointer to that index.
 * RU: Если ref указывает на нативный числовой слот, вернуть заимствованный указатель на индекс. */
const unsigned long *fvm_variable_ref_index(const FVM_Object *ref);


/* [GC:] EN: Capture an existing own slot of args[0]. Resolve the evaluated key in args[1]
 * as a native numeric index or a named property. Transfer the reference operand into out.
 * RU: Захватить собственный слот args[0], разрешив вычисленный ключ args[1] как числовой
 * индекс нативной таблицы или имя свойства. Вернуть ссылочный операнд в out. */
extern "C" FVM_Result fvm_capture_variable_ref(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* EN/RU: Make a bounded local FIFO for references and scalars. / Создать локальную очередь ссылок и скаляров. */
FVM_Result fvm_create_ref_channel(FVM_Context *, const FVM_Value *, FVM_Value *);
#endif
