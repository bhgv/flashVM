/* Private bounded parser state; generated grammar uses only these actions. */
#ifndef SBLMAP_STATE_H
#define SBLMAP_STATE_H
#include "sblmap.h"

#define SBLMAP_MAX_INPUT (4u * 1024u * 1024u)  /* Whole input byte admission. */
#define SBLMAP_MAX_FILES 4096u                 /* File-table allocation bound. */
#define SBLMAP_MAX_PC 262144u                  /* Expanded logical-PC allocation bound. */
#define SBLMAP_MAX_TEXT 1024u                  /* Module/file/header/PC record byte bound. */
#define SBLMAP_NUMBER_MAX UINT32_C(2147483647) /* Nonnegative Dis-int representation. */


/* Grammar action roles; these are not a wire format. */
enum SblRole
{
    SBL_HEADER,
    SBL_MODULE,
    SBL_FILES,
    SBL_FILE,
    SBL_PCS,
    SBL_PC,
    SBL_TAIL
};


/* Decode one nonempty line while keeping the first error sticky. */
void sblmap_line(int role, const char *text);


/* Capture the original line span, including leading spaces skipped by Coco/R. */
void sblmap_capture(int role, size_t position, size_t length);


/* Retain a bounded-record failure before copying any oversized token. */
void sblmap_record_limit(void);


/* Record prefix termination after consuming a line-ending token. */
void sblmap_end(int role, size_t offset);


/* Dynamic grammar resolvers bounded by the declared counts. */
int sblmap_need_files(void);


int sblmap_need_pc(void);


/* Record a generated syntax error without console output. */
void sblmap_syntax(int code, int line, int column, long offset);
#endif
