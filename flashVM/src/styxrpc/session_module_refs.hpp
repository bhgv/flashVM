/* Copyright (c) 2026 bhgv
 * EN: Adapt scoped module-variable records to ordinary ActionScript references.
 * RU: Адаптация контекстных записей переменных к обычным ссылкам ActionScript.
 */
#ifndef FVM_STYXRPC_SESSION_MODULE_REFS_HPP
#define FVM_STYXRPC_SESSION_MODULE_REFS_HPP
#include "session_named_values.hpp"


/* EN: Inspect a captured getter and encode its exact hosted export into wire.
 * With an unrelated reference, return OK and zero bytes.
 * RU: Проверить захваченный геттер и закодировать его точный экспорт в wire.
 * Для ссылки другого вида вернуть OK и ноль байтов. */
FVM_Result fvm_named_module_ref_encode(FVM_Context *context, const FVM_NamedRoute &route,
    const FVM_Value *value, unsigned char *wire, size_t capacity, size_t *used);


/* EN: Validate scope and materialize a transparent alias for the original publication.
 * RU: Проверить область и создать прозрачную ссылку на исходную публикацию. */
FVM_Result fvm_named_module_ref_decode(FVM_Context *context, const FVM_NamedRoute &route,
    const unsigned char *wire, size_t size, FVM_Value *out);
#endif
