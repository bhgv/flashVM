/* Copyright (c) 2026 bhgv
 * EN: Generate a scalar remote owner adapter from the existing C interface catalog.
 * RU: Генерация scalar remote owner adapter через существующий C interface catalog.
 */
#ifndef LIMBO_REMOTE_OWNER_H
#define LIMBO_REMOTE_OWNER_H
#include "signature.h"
#ifdef __cplusplus
extern "C" {
#endif


/* EN: Borrow catalog, include basename and provider-local implementation path.
 * Support exactly one fn(int):int in one module, with constants/scalar aliases, but no includes/ADTs.
 * Return 1/0 and optional error text; output=NULL preflights without writing.
 * Reject unsupported input before output. Stream failures may leave a prefix;
 * caller owns publication and catalog lifetime. No VM execution or wire I/O.
 * RU: Заимствовать catalog, include basename и provider-local implementation path.
 * Поддержать ровно одну fn(int):int в одном module, с constants/scalar aliases, без includes/ADT.
 * Вернуть 1/0 и optional error; output=NULL проверяет без записи. Unsupported
 * input отклоняется до output. Stream error может оставить prefix; publication
 * и lifetime принадлежат caller. Без VM execution и wire I/O.
 */
int limbo_remote_owner(FILE *output, const LimboInterface *catalog,
                       const char *include_name, const char *implementation,
                       char *error, size_t capacity);
/* EN/RU: Emit client interface or implementation for the same scalar ABI.
 * Emit implementation using the generated include basename. NULL preflights.
 * Выдать клиентский интерфейс или реализацию scalar ABI, NULL только проверяет. */
int limbo_remote_client(FILE *output, const LimboInterface *catalog, const char *include_name,
    int interface_only, char *error, size_t capacity);



/* EN/RU: Emit a same-interface shim for a trusted pair of VFS routes.
 * Сгенерировать модуль исходного интерфейса для выданной пары путей VFS. */
int limbo_styx_client(FILE *output, const LimboInterface *catalog, const char *include_name,
    const char *request, const char *reply, const char *request_tag, const char *reply_tag,
    char *error, size_t capacity);


/* EN/RU: Emit or preflight one owner Prog. The implementation path is owner-local.
 * Выдать или проверить одноразового владельца. Путь реализации локален владельцу. */
int limbo_styx_owner(FILE *output, const LimboInterface *catalog, const char *include_name,
    const char *implementation, const char *request, const char *reply,
    const char *request_tag, const char *reply_tag, char *error, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
