/* Caller-owned source-map views of the Limbo .sbl 2.1 PC prefix. */
#ifndef LIMBO_SBLMAP_H
#define LIMBO_SBLMAP_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
/* Distinguish malformed input, representation limits and unsupported versions. */
typedef enum SblResult
{
    SBL_OK,
    SBL_INVALID,
    SBL_LIMIT,
    SBL_NOMEM,
    SBL_UNSUPPORTED,
    SBL_NOT_FOUND
} SblResult;
/* Owns copied module/file names and expanded PC entries; release with sblmap_destroy. */
typedef struct SblMap SblMap;


/* Source coordinates are producer values; zero is preserved, not invented source. */
typedef struct SblEntry
{
    uint32_t start_file;   /* Zero-based file-table index at the start. */
    uint32_t start_line;   /* Authored source line, usually one-based; zero is unknown. */
    uint32_t start_column; /* Authored start position; no editor coordinate conversion. */
    uint32_t stop_file;    /* Zero-based file-table index at the stop. */
    uint32_t stop_line;    /* Authored stop line; may differ from the start. */
    uint32_t stop_column;  /* Authored stop position. */
    uint32_t statement;    /* Producer block/statement grouping identity. */
} SblEntry;


/* Copy a bounded text map. Preserve *out on all failures; error is optional caller storage.
     * Input/output/diagnostic storage must be accessible and disjoint.
     * Generated parser calls require external serialization. No callbacks or VM access.
     * Types/functions/globals after prefix_bytes are opaque, not validated by this API. */
SblResult sblmap_parse(const void *data, size_t size, SblMap **out, char *error, size_t capacity);


/* Release owned storage; null is safe. Do not query a map concurrently with destruction. */
void sblmap_destroy(SblMap *map);


/* Borrow a NUL-terminated module name until destroy; null map returns null. */
const char *sblmap_module(const SblMap *map);


/* Return file-table cardinality; null map returns zero. */
size_t sblmap_files(const SblMap *map);


/* Borrow one file spelling; invalid map/index returns null without filesystem access. */
const char *sblmap_file(const SblMap *map, size_t index);


/* Return logical Dis-PC cardinality; null map returns zero. */
size_t sblmap_count(const SblMap *map);


/* Return bytes through the PC prefix, including its last line terminator. */
size_t sblmap_prefix_bytes(const SblMap *map);


/* Copy one entry; invalid arguments or NOT_FOUND preserve output. PC is a logical index. */
SblResult sblmap_entry(const SblMap *map, size_t pc, SblEntry *out);


/* Find first entry at/after first_pc whose start file/line match exactly.
     * NOT_FOUND and argument errors preserve *pc; no nearest-line or breakpoint policy is implied.
     */
SblResult sblmap_find(const SblMap *map, size_t file, uint32_t line, size_t first_pc, size_t *pc);


/* Find the first PC >= first_pc containing an authored position (inclusive endpoints).
     * Only ordered spans within one file and with nonzero endpoint lines participate.
     * Line must be nonzero; column zero is a valid producer position.
     * INVALID/NOT_FOUND preserve *pc. No path basename matching or filesystem access. */
SblResult sblmap_position(const SblMap *map, size_t file, uint32_t line, uint32_t column, size_t first_pc, size_t *pc);
/* Resolve an editor line to the earliest authored start line >= line and <= last_line.
     * Among equal lines choose the smallest PC >= first_pc. Zero lines never participate.
     * Explicit inclusive last_line bounds forward sliding; no hidden 80-line heuristic.
     * INVALID/NOT_FOUND preserve *pc. This selects a candidate, not an installed breakpoint. */
SblResult
sblmap_breakpoint(const SblMap *map, size_t file, uint32_t line, uint32_t last_line, size_t first_pc, size_t *pc);


/* Find the first later PC with a different producer statement identity.
     * This is a linear map query, NOT runtime stepping, control-flow traversal or StepOver.
     * The host must observe actual execution/frame/module changes through Dis debug controls.
     * INVALID/NOT_FOUND preserve *next_pc; an absent starting PC returns NOT_FOUND. */
SblResult sblmap_next_statement(const SblMap *map, size_t pc, size_t *next_pc);
#ifdef __cplusplus
}
#endif
#endif
