#ifndef LIMBO_SOURCE_POSITION_H
#define LIMBO_SOURCE_POSITION_H
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif


/* Normalized physical coordinates for an exact original UTF-8 source snapshot. */
typedef struct LimboSourcePosition
{
    size_t byte_offset; /* Zero-based original byte boundary, including EOF. */
    size_t line;        /* One-based physical line; CRLF/CR/LF each end one line. */
    size_t column;      /* One-based Unicode scalar column; tab counts as one. */
} LimboSourcePosition;


/* Validate borrowed text[0:size] and resolve offset without modifying text.
 * Return 1 and replace *out on success; return 0 and preserve *out for invalid
 * UTF-8/NUL, offset inside a scalar/CRLF, null pointers or size above 2 MiB.
 * Empty text may be null. EOF is valid. No allocation, parser state, I/O or
 * blocking occurs; independent calls are reentrant. Columns are not graphemes,
 * display cells, UTF-16 units or legacy Coco/R scanner diagnostic columns. */
int limbo_source_position(const char *text, size_t size, size_t offset, LimboSourcePosition *out);
#ifdef __cplusplus
}
#endif
#endif
