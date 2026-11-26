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


/* [GC:] EN: Capture an existing own slot of args[0], converting the key in args[1] once.
 * Returns a reference in out. The separate raw numeric table namespace is excluded.
 * RU: Захватить существующий собственный слот args[0], один раз преобразовав ключ args[1].
 * Возвращает ссылку в out. Отдельное пространство нативных числовых индексов исключено. */
extern "C" FVM_Result fvm_capture_variable_ref(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* EN/RU: Make a bounded local FIFO for references and scalars. / Создать локальную очередь ссылок и скаляров. */
FVM_Result fvm_create_ref_channel(FVM_Context *, const FVM_Value *, FVM_Value *);
#endif
