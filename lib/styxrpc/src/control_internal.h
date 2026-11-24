/* Copyright (c) 2026 bhgv
 * EN: Shared node IDs of the standalone control tree and mixed module root.
 * RU: Общие ID узлов самостоятельного управления и корня смешанного модуля.
 */
#ifndef STYXRPC_CONTROL_INTERNAL_H
#define STYXRPC_CONTROL_INTERNAL_H
enum
{
    CONTROL_ROOT = 0, /* Directory holding the two files. */
    CONTROL_UPLOAD,   /* Counted write-only upload. */
    CONTROL_STATE     /* Read snapshot, write commands. */
};
#endif
