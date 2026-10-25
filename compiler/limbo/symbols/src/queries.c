/* Immutable source-map queries for host debugger adapters; no parser or VM state. */
#include "sblmap.h"


/* Compare coordinates lexicographically without overflow or signed subtraction. */
static int before(uint32_t line, uint32_t column, uint32_t other_line, uint32_t other_column)
{
    return line < other_line || (line == other_line && column < other_column);
}


/* Match one-file authored ranges, preserving caller output unless a candidate exists. */
SblResult sblmap_position(const SblMap *map, size_t file, uint32_t line, uint32_t column, size_t first_pc, size_t *pc)
{
    size_t i;
    SblEntry entry;
    if (!map || !pc || file >= sblmap_files(map) || !line)
    {
        return SBL_INVALID;
    }
    for (i = first_pc; i < sblmap_count(map); ++i)
    {
        if (sblmap_entry(map, i, &entry) != SBL_OK)
        {
            return SBL_INVALID;
        }
        if (entry.start_file != file || entry.stop_file != file || !entry.start_line || !entry.stop_line)
        {
            continue;
        }
        if (before(line, column, entry.start_line, entry.start_column) ||
            before(entry.stop_line, entry.stop_column, line, column))
        {
            continue;
        }
        *pc = i;
        return SBL_OK;
    }
    return SBL_NOT_FOUND;
}


/* Select the closest forward authored line, with deterministic PC ordering for ties. */
SblResult
sblmap_breakpoint(const SblMap *map, size_t file, uint32_t line, uint32_t last_line, size_t first_pc, size_t *pc)
{
    size_t i, chosen = 0;
    uint32_t chosen_line = 0;
    SblEntry entry;
    if (!map || !pc || file >= sblmap_files(map) || !line || last_line < line)
    {
        return SBL_INVALID;
    }
    for (i = first_pc; i < sblmap_count(map); ++i)
    {
        if (sblmap_entry(map, i, &entry) != SBL_OK)
        {
            return SBL_INVALID;
        }
        if (entry.start_file != file || entry.start_line < line || entry.start_line > last_line)
        {
            continue;
        }
        if (!chosen_line || entry.start_line < chosen_line)
        {
            chosen = i;
            chosen_line = entry.start_line;
        }
    }
    if (!chosen_line)
    {
        return SBL_NOT_FOUND;
    }
    *pc = chosen;
    return SBL_OK;
}


/* Inspect adjacent logical mappings only; do not predict branches, calls or frame returns. */
SblResult sblmap_next_statement(const SblMap *map, size_t pc, size_t *next_pc)
{
    size_t i;
    SblEntry initial, entry;
    SblResult result;
    if (!map || !next_pc)
    {
        return SBL_INVALID;
    }
    result = sblmap_entry(map, pc, &initial);
    if (result != SBL_OK)
    {
        return result;
    }
    /* A valid pc is strictly below the bounded count, so pc + 1 cannot wrap. */
    for (i = pc + 1; i < sblmap_count(map); ++i)
    {
        if (sblmap_entry(map, i, &entry) != SBL_OK)
        {
            return SBL_INVALID;
        }
        if (entry.statement != initial.statement)
        {
            *next_pc = i;
            return SBL_OK;
        }
    }
    return SBL_NOT_FOUND;
}
