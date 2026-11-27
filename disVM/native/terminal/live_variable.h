/* Copyright (c) 2026 bhgv
 * EN: Existing indexed VFS types and the counted string cell file shared by both peers.
 * RU: Действующие типы индексной VFS и общий файл строковой ячейки с длиной.
 */
#ifndef STYX_LIVE_VARIABLE_H
#define STYX_LIVE_VARIABLE_H

#define STYX_LIVE_INT_ARRAY "array of int\n"  /* Fixed/native Int32 positional cells. */
#define STYX_LIVE_BIG_VECTOR "vector of big\n" /* Signed big positions in the Dis vector. */
#define STYX_LIVE_BIG_TABLE "table of big\n"  /* Numeric access to the table's positional/keyed space. */
#define STYX_LIVE_RECORD "record\n"           /* Fixed directory of named capabilities. */
#define STYX_LIVE_BIG "big\n"                /* One signed big S-record in .val. */
#define STYX_LIVE_STRING "string\n"           /* One counted UTF-8 or nil record in .val. */
#define STYX_LIVE_STRING_LIMIT 1024U            /* Maximum payload bytes excluding the S-record header. */
#define STYX_LIVE_STRING_FRAME (STYX_LIVE_STRING_LIMIT + 8U) /* Bounded scratch for header and payload. */
#endif
