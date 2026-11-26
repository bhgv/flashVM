/* Copyright (c) 2026 bhgv
 * EN: Wire offsets for one executable node request.
 * RU: Смещения полей запроса к исполняемому узлу.
 */
#ifndef STYXRPC_NODE_EXEC_WIRE_H
#define STYXRPC_NODE_EXEC_WIRE_H

#define NODE_EXEC_WORD 4U /* Width of each unsigned big-endian count. */
#define NODE_EXEC_HEADER (2U * NODE_EXEC_WORD) /* Counted path, then in-place byte capacity. */

#endif
