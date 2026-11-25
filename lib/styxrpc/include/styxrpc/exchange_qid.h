/* Copyright (c) 2026 bhgv
 * EN: Identify service-only message Qids without depending on the hosted SDK.
 * RU: Определение служебных Qid сообщений без зависимости от SDK сервера.
 */
#ifndef STYXRPC_EXCHANGE_QID_H
#define STYXRPC_EXCHANGE_QID_H
#include <stdint.h>

#define STYXRPC_MESSAGE_NODE (UINT64_MAX - 1) /* Reserve descending Qids; UINT64_MAX stays unused. */
#define STYXRPC_MESSAGE_ROOTS_MAX 16U /* Named message roots per hosted Styx server. */
#define STYXRPC_EXCHANGE_QID(qid) (STYXRPC_MESSAGE_NODE - (uint64_t)(qid) < STYXRPC_MESSAGE_ROOTS_MAX) /* True for a service Qid. */

#endif
