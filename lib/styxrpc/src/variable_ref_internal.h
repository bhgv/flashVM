/* Copyright (c) 2026 bhgv
 * EN: Connect reference publication to the program registry lock.
 * RU: Связать публикацию ссылки с блокировкой реестра программ.
 */
#ifndef STYXRPC_VARIABLE_REF_INTERNAL_H
#define STYXRPC_VARIABLE_REF_INTERNAL_H
#include "styxrpc/variable_ref.h"


/* EN/RU: Publish under the registry lock. / Опубликовать под блокировкой реестра. */
uint32_t styxrpc_program_int_ref_grant(StyxRpcPrograms *, const StyxIntVariable *, int writable);


/* EN/RU: Revoke under the registry lock. / Отозвать под блокировкой реестра. */
int styxrpc_program_int_ref_revoke(StyxRpcPrograms *, const StyxRpcIntRef *);
#endif
