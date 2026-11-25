/* Copyright (c) 2026 bhgv
 * EN: Registered UniBus message identities for mixed-value RPC channels.
 * RU: Идентификаторы сообщений UniBus для RPC-каналов с разными типами.
 */
#ifndef STYXRPC_CHANNEL_MESSAGES_H
#define STYXRPC_CHANNEL_MESSAGES_H
#include "styxrpc/messages.h"

#define STYXRPC_CHANNEL_MESSAGE_VERSION 1U             /* Version of each schema below. */
#define STYXRPC_CHANNEL_MESSAGE_INT "styxrpc.int32"     /* One STYXRPC_VALUE_INT managed child. */
#define STYXRPC_CHANNEL_MESSAGE_REAL "styxrpc.real64"   /* One STYXRPC_VALUE_REAL managed child. */
#define STYXRPC_CHANNEL_MESSAGE_BOOL "styxrpc.bool"    /* One byte, 0 or 1. */
#define STYXRPC_CHANNEL_MESSAGE_TEXT "styxrpc.text"    /* Counted UTF-8/WTF-8 bytes, including NUL. */
#define STYXRPC_CHANNEL_MESSAGE_NULL "styxrpc.null"    /* Empty payload. */
#define STYXRPC_CHANNEL_MESSAGE_REF "styxrpc.ref.int32" /* One StyxRpcIntRef from this session's issuer. */

#endif
