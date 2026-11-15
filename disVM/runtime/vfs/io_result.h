/* Copyright (c) 2026 bhgv
 * EN: Native chunk acknowledgements, stop reasons and post-failure metadata hints.
 * RU: Native подтверждения чанков, причины остановки и metadata после отказа.
 */
#ifndef DVM_VFS_IO_RESULT_H
#define DVM_VFS_IO_RESULT_H
#include "file_port.h"
#include <stdint.h>
#define DVM_IO_PREFIX 1U /* Count is an acknowledged prefix; never inferred on legacy failure. */
#define DVM_IO_UNCERTAIN 2U /* Effects beyond the prefix are unknown; automatic replay is forbidden. */
#define DVM_IO_PROBED 4U /* A post-failure metadata query was attempted, possibly unsupported. */
#define DVM_IO_MODE 8U /* Observed mode is valid metadata, not effective-access authority. */
#define DVM_IO_CHANGED 16U /* Backend reports a change generation; no permission grant or EOF. */
#define DVM_IO_FLAGS 31U /* All currently defined native result flags. */


/* EN/RU: Distinct stop causes; no failed-stack condition is silently EOF.
 * Разные причины остановки; отказ стопки не превращается в EOF. */
typedef enum DVM_IoStop
{
    DVM_IO_NONE = 0, /* Ordinary successful progress. */
    DVM_IO_EOF, /* Read source ended; not a write result. */
    DVM_IO_DENIED, /* Operation reported access refusal, not inferred from stat. */
    DVM_IO_AGAIN, /* Temporary inability to make progress. */
    DVM_IO_NO_PROGRESS, /* Positive write request acknowledged zero bytes. */
    DVM_IO_FAILURE, /* Operation failed with no more specific cause. */
    DVM_IO_CANCELLED, /* Operation stopped by cancellation. */
    DVM_IO_INVALID /* Malformed callback result; acknowledgement cannot be trusted. */
} DVM_IoStop;


/* EN/RU: Scalar native result, never managed pointers. Backend supplies status,
 * count, stop and PREFIX/UNCERTAIN/CHANGED; dispatcher owns probe fields/flags.
 * Scalar native result, без managed pointers. Backend задаёт status/count/stop
 * и PREFIX/UNCERTAIN/CHANGED; dispatcher владеет probe fields/flags. */
typedef struct DVM_IoResult
{
    size_t count; /* Confirmed leading bytes, meaningful only with PREFIX. */
    FVM_Result status; /* Original operation status, independent from metadata status. */
    DVM_IoStop stop; /* Why this chunk ended; never derived from metadata mode. */
    unsigned flags; /* Prefix certainty, uncertain tail, metadata and change hints. */
    FVM_Result probe_status; /* Diagnostic post-failure info result; UNSUPPORTED before probe. */
    unsigned long observed_mode; /* Low mode bits when MODE is set; not an access decision. */
    uint64_t generation; /* Optional backend change generation within its session. */
} DVM_IoResult;
#endif
