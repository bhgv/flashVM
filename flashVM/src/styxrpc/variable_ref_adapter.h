/* Copyright (c) 2026 bhgv
 * EN: Bind a received StyxRPC reference to the native AS variable adapter.
 * RU: Связать полученную ссылку StyxRPC с адаптером переменных AS.
 */
#ifndef FVM_STYXRPC_VARIABLE_REF_ADAPTER_H
#define FVM_STYXRPC_VARIABLE_REF_ADAPTER_H
#include "flashvm/styxrpc_session.h"
#include "styxrpc/variable_ref.h"


/* [GC:] EN/RU: Capture a named variable operand. / Захватить ссылочный операнд. */
FVM_Result fvm_capture_variable_ref(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* EN: Return one only for the SDK's native Int32 file getter.
 * RU: Вернуть единицу только для нативного getter файла Int32 из SDK. */
int fvm_styxrpc_int_variable_getter(FVM_NativeFunction getter);


/* [GC:] EN/RU: Attach the resolved fid to value. / Привязать разрешённый fid к value. */
FVM_Result fvm_styxrpc_int_ref_bind(FVM_Context *, FVM_Object *, struct StyxClient *, const StyxRpcIntRef *);


/* EN: Write the original descriptor to bytes (capacity bytes) only for its own session.
 * RU: Записать исходный дескриптор в bytes (capacity байт) только для его сессии. */
int fvm_styxrpc_encode_int_ref(FVM_StyxRpcSession *binding, const FVM_Value *value,
    unsigned char *bytes, size_t capacity);


/* [ASINC] [GC:] EN: Open a descriptor of size bytes on binding and write a live operand to out.
 * RU: Открыть дескриптор size байт в binding и выдать живой операнд в out. */
FVM_Result fvm_styxrpc_decode_int_ref(FVM_Context *context, FVM_StyxRpcSession *binding,
    const void *bytes, size_t size, FVM_Value *out);


/* [GC:] EN/RU: Decode bytes on the existing issuer session. / Декодировать байты в сессии владельца. */
FVM_Result fvm_styxrpc_accept_int_ref(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);

/* EN/RU: Record immutable wire identity. / Сохранить неизменяемую сетевую идентичность. */
FVM_Result fvm_styxrpc_remember_int_ref(FVM_Object *, struct StyxClient *, const StyxRpcIntRef *);


/* [GC:] EN/RU: Encode a ref on its issuer session. / Закодировать ссылку в её сессии. */
FVM_Result fvm_styxrpc_int_ref_bytes(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);
/* [GC:] EN: Wrap a byte carrier explicitly selected for this issuer session.
 * RU: Обернуть байтовый канал, явно выбранный для сессии владельца. */
FVM_Result fvm_styxrpc_int_ref_channel(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);
#endif
