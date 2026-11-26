/* Copyright (c) 2026 bhgv
 * EN: Bind writable AS slots to cells on the attached StyxRPC session.
 * RU: Привязывать записываемые слоты AS к ячейкам в подключённой сессии.
 */
#ifndef FVM_NAMED_REFS_HPP
#define FVM_NAMED_REFS_HPP
#include "flashvm/styxrpc_session.h"
#include "styxrpc/named_cell.h"

struct FVM_AccessorEntry;
struct FVM_NamedRoute;

/* [GC:] EN: Identify a named cell getter. Its token is borrowed through entry's root.
 * RU: Распознать getter именованной ячейки. Жетон заимствован через корень entry. */
const unsigned char *fvm_named_cell_identity(const FVM_AccessorEntry &entry, struct StyxClient **issuer);


/* [GC:] EN: Resolve ref to a live cell on session, publishing a fresh slot if needed.
 * RU: Получить живую ячейку ref на session, при необходимости опубликовав новый слот. */
FVM_Result fvm_named_ref_token(FVM_Context *, const FVM_NamedRoute &, const FVM_Value *, unsigned char *token);

/* [GC:] EN: Resolve a counted token to a new transparent reference operand.
 * RU: Разрешить жетон с длиной в новый прозрачный ссылочный операнд. */
FVM_Result fvm_named_ref_bind(FVM_Context *, const FVM_NamedRoute &, const void *token, size_t, FVM_Value *);
#endif
