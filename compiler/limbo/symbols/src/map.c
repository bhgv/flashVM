/* Bounded .sbl 2.1 prefix ownership and checked compressed-coordinate decoding. */
#include "sblmapp.h"
#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* One immutable successfully parsed map, independent of the original input. */
struct SblMap
{
    char *module;      /* Owned module spelling. */
    char **files;      /* Owned table and individual names. */
    size_t file_count; /* Declared number of file spellings. */
    SblEntry *entries; /* Owned expanded PC records. */
    size_t count;      /* Declared PC count. */
    size_t prefix;     /* End of the last source-map prefix line. */
};

/* Generated parser globals are serialized by the embedding caller. */
static SblMap *active;
static const unsigned char *active_input; /* Borrowed copied bytes for line capture. */
static size_t active_size;                /* Bound every scanner-reported byte span. */
static size_t file_at, pc_at;
static uint32_t last_file, last_line;
static SblResult failure;
static char *message;
static size_t message_capacity;


extern int sblmap_parser_run(unsigned char *, int);
extern Error_Func Custom_Error;


/* Preserve the first diagnostic and failure status. */
static void reject(SblResult status, const char *text)
{
    if (failure != SBL_OK)
    {
        return;
    }
    failure = status;
    if (message && message_capacity)
    {
        snprintf(message, message_capacity, "%s", text);
    }
}


/* Read a decimal Dis-int without signed overflow or partial-number acceptance. */
static int number(const char **cursor, uint32_t *out)
{
    const char *s = *cursor;
    uint32_t value = 0;
    if (*s < '0' || *s > '9')
    {
        return 0;
    }
    while (*s >= '0' && *s <= '9')
    {
        unsigned digit = (unsigned)(*s++ - '0');
        if (value > (SBLMAP_NUMBER_MAX - digit) / 10)
        {
            return 0;
        }
        value = value * 10 + digit;
    }
    *cursor = s;
    *out = value;
    return 1;
}


/* Copy one bounded owned text field, recording allocation failure. */
static char *copy(const char *text)
{
    size_t size = strlen(text) + 1;
    char *result = (char *)malloc(size);
    if (!result)
    {
        reject(SBL_NOMEM, "symbol text allocation failed");
    }
    else
    {
        memcpy(result, text, size);
    }
    return result;
}


/* Read one [file:][line.]position using the producer's inherited state. */
static int position(const char **cursor, uint32_t *file, uint32_t *line, uint32_t *column)
{
    uint32_t value;
    if (!number(cursor, &value))
    {
        return 0;
    }
    if (**cursor == ':')
    {
        last_file = value;
        ++*cursor;
        if (!number(cursor, &value))
        {
            return 0;
        }
    }
    if (**cursor == '.')
    {
        last_line = value;
        ++*cursor;
        if (!number(cursor, &value))
        {
            return 0;
        }
    }
    if (last_file >= active->file_count)
    {
        return 0;
    }
    *file = last_file;
    *line = last_line;
    *column = value;
    return 1;
}


/* Publish a complete expanded PC record only after both positions and grouping parse. */
static int pc_line(const char *text)
{
    SblEntry entry;
    const char *s = text;
    if (!position(&s, &entry.start_file, &entry.start_line, &entry.start_column) || *s++ != ',')
    {
        return 0;
    }
    if (!position(&s, &entry.stop_file, &entry.stop_line, &entry.stop_column) || *s++ != ' ')
    {
        return 0;
    }
    if (!number(&s, &entry.statement) || *s)
    {
        return 0;
    }
    active->entries[pc_at++] = entry;
    return 1;
}


/* Apply grammar actions; count declarations bound every subsequent table write. */
void sblmap_line(int role, const char *text)
{
    const char *s = text;
    uint32_t count;
    if (failure != SBL_OK || role == SBL_TAIL)
    {
        return;
    }
    if (strlen(text) >= SBLMAP_MAX_TEXT)
    {
        reject(SBL_LIMIT, "symbol record too long");
        return;
    }
    switch (role)
    {
    case SBL_HEADER:
        if (strcmp(text, "limbo .sbl 2.1"))
        {
            reject(SBL_UNSUPPORTED, "expected limbo .sbl 2.1");
        }
        break;
    case SBL_MODULE:
        active->module = copy(text);
        break;
    case SBL_FILES:
    case SBL_PCS:
        if (!number(&s, &count) || *s)
        {
            reject(SBL_INVALID, "invalid table count");
            break;
        }
        if (count > (role == SBL_FILES ? SBLMAP_MAX_FILES : SBLMAP_MAX_PC))
        {
            reject(SBL_LIMIT, "symbol table limit exceeded");
            break;
        }
        if (role == SBL_FILES)
        {
            active->file_count = count;
            if (count)
            {
                active->files = (char **)calloc(count, sizeof(*active->files));
            }
            if (count && !active->files)
            {
                reject(SBL_NOMEM, "file table allocation failed");
            }
        }
        else
        {
            active->count = count;
            if (count)
            {
                active->entries = (SblEntry *)calloc(count, sizeof(*active->entries));
            }
            if (count && !active->entries)
            {
                reject(SBL_NOMEM, "PC table allocation failed");
            }
        }
        break;
    case SBL_FILE:
        if (file_at >= active->file_count)
        {
            reject(SBL_INVALID, "excess file record");
        }
        else
        {
            active->files[file_at++] = copy(text);
        }
        break;
    case SBL_PC:
        if (pc_at >= active->count || !pc_line(text))
        {
            reject(SBL_INVALID, "invalid compressed source record");
        }
        break;
    default:
        reject(SBL_INVALID, "invalid parser action");
        break;
    }
}


/* Preserve complete raw lines despite the generator's mandatory skipped spaces. */
void sblmap_capture(int role, size_t position, size_t length)
{
    char text[SBLMAP_MAX_TEXT];
    size_t start = position;
    if (failure != SBL_OK || role == SBL_TAIL)
    {
        return;
    }
    if (position > active_size || length > active_size - position)
    {
        reject(SBL_INVALID, "invalid scanner span");
        return;
    }
    while (start && active_input[start - 1] != '\n')
    {
        --start;
    }
    length += position - start;
    if (length >= sizeof(text))
    {
        sblmap_record_limit();
        return;
    }
    memcpy(text, active_input + start, length);
    text[length] = 0;
    sblmap_line(role, text);
}


/* Stop oversized prefix records before copying their spelling. */
void sblmap_record_limit(void)
{
    reject(SBL_LIMIT, "symbol record too long");
}


/* Keep an exact byte boundary while treating later symbol tables as opaque text. */
void sblmap_end(int role, size_t offset)
{
    if (failure == SBL_OK && (role == SBL_PC || role == SBL_PCS))
    {
        active->prefix = offset;
    }
}


/* Continue only while a declared file remains and no failure is sticky. */
int sblmap_need_files(void)
{
    return failure == SBL_OK && file_at < active->file_count;
}


/* Continue only while a declared PC remains and no failure is sticky. */
int sblmap_need_pc(void)
{
    return failure == SBL_OK && pc_at < active->count;
}


/* Retain generated syntax coordinates without exposing callback state. */
void sblmap_syntax(int code, int line, int column, long offset)
{
    (void)offset;
    if (failure == SBL_OK)
    {
        failure = SBL_INVALID;
        if (message && message_capacity)
        {
            snprintf(message, message_capacity, "sbl:%d:%d: syntax E%d", line, column, code);
        }
    }
}


/* Free partial or complete ownership, including an allocation-failed table. */
void sblmap_destroy(SblMap *map)
{
    size_t i;
    if (!map)
    {
        return;
    }
    if (map->files)
    {
        for (i = 0; i < map->file_count; ++i)
        {
            free(map->files[i]);
        }
    }
    free(map->files);
    free(map->entries);
    free(map->module);
    free(map);
}


/* Serialize generated state, copy input, and publish the map only on complete prefix success. */
SblResult sblmap_parse(const void *data, size_t size, SblMap **out, char *error, size_t capacity)
{
    unsigned char *text;
    SblMap *map;
    SblResult result;
    int ran;
    if (error && capacity)
    {
        error[0] = 0;
    }
    if (!out || (!data && size))
    {
        return SBL_INVALID;
    }
    if (size > SBLMAP_MAX_INPUT)
    {
        return SBL_LIMIT;
    }
    if (size && memchr(data, 0, size))
    {
        return SBL_INVALID;
    }
    text = (unsigned char *)calloc(size + 1, 1);
    map = (SblMap *)calloc(1, sizeof(*map));
    if (!text || !map)
    {
        free(text);
        free(map);
        return SBL_NOMEM;
    }
    if (size)
    {
        memcpy(text, data, size);
    }
    active = map;
    active_input = text;
    active_size = size;
    file_at = pc_at = 0;
    last_file = last_line = 0;
    failure = SBL_OK;
    message = error;
    message_capacity = capacity;
    Custom_Error = sblmap_syntax;
    ran = sblmap_parser_run(text, (int)size);
    if (!ran)
    {
        reject(SBL_NOMEM, "scanner allocation failed");
    }
    if (failure == SBL_OK && (!Successful() || !map->module || file_at != map->file_count || pc_at != map->count))
    {
        reject(SBL_INVALID, "incomplete symbol prefix");
    }
    result = failure;
    Custom_Error = NULL;
    active = NULL;
    active_input = NULL;
    active_size = 0;
    message = NULL;
    message_capacity = 0;
    free(text);
    if (result == SBL_OK)
    {
        *out = map;
    }
    else
    {
        sblmap_destroy(map);
    }
    return result;
}


/* Borrow immutable metadata only while the caller retains its map owner. */
const char *sblmap_module(const SblMap *map)
{
    return map ? map->module : NULL;
}


/* Return file count without consulting mutable parser globals. */
size_t sblmap_files(const SblMap *map)
{
    return map ? map->file_count : 0;
}


/* Borrow an immutable file-table spelling. */
const char *sblmap_file(const SblMap *map, size_t index)
{
    return map && index < map->file_count ? map->files[index] : NULL;
}


/* Return the logical Dis-PC count. */
size_t sblmap_count(const SblMap *map)
{
    return map ? map->count : 0;
}


/* Return the precise prefix byte extent. */
size_t sblmap_prefix_bytes(const SblMap *map)
{
    return map ? map->prefix : 0;
}


/* Copy one logical-PC mapping without changing output on error. */
SblResult sblmap_entry(const SblMap *map, size_t pc, SblEntry *out)
{
    if (!map || !out)
    {
        return SBL_INVALID;
    }
    if (pc >= map->count)
    {
        return SBL_NOT_FOUND;
    }
    *out = map->entries[pc];
    return SBL_OK;
}


/* Find an exact authored start line, leaving breakpoint selection to the host. */
SblResult sblmap_find(const SblMap *map, size_t file, uint32_t line, size_t first_pc, size_t *pc)
{
    size_t i;
    if (!map || !pc || file >= map->file_count)
    {
        return SBL_INVALID;
    }
    for (i = first_pc; i < map->count; ++i)
    {
        if (map->entries[i].start_file == file && map->entries[i].start_line == line)
        {
            *pc = i;
            return SBL_OK;
        }
    }
    return SBL_NOT_FOUND;
}
