/* Copyright (c) 2026 bhgv
 * EN: Internal mapping from a registered node to an indexed variable file operation.
 * RU: Внутреннее сопоставление зарегистрированного узла и операции коллекции.
 */
#ifndef STYXRPC_COLLECTION_INTERNAL_H
#define STYXRPC_COLLECTION_INTERNAL_H
#include "styxrpc/collection.h"

typedef enum CollectionOperation
{
    CollectionIndex,  /* Open selector and write one index/key. */
    CollectionValue,  /* Read or write selected value. */
    CollectionLength, /* Read decimal length. */
    CollectionType,   /* Read collection type description. */
    CollectionInsert, /* Insert at numeric index. */
    CollectionPush,   /* Append one positional element. */
    CollectionPop,    /* Remove the last positional element. */
    CollectionAdd     /* Add a new numeric key. */
} CollectionOperation;


/* EN: Fill one registration's callback descriptor, borrowing array and its owning session.
 * RU: Заполнить колбэки одного узла, заимствуя коллекцию и владеющую сессию. */
void styxrpc_collection_bind(StyxRpcRegistration *entry, StyxRpcPrograms *owner,
    StyxIntArray *array, CollectionOperation operation);

#endif
