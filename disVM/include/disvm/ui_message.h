/* Copyright (c) 2026 bhgv
 * EN: Copied IDE message values and wire bounds, independent of VM and channels.
 * RU: Копируемые значения сообщений IDE и границы формата, без VM и каналов.
 */
#ifndef DISVM_UI_MESSAGE_H
#define DISVM_UI_MESSAGE_H
#include <stddef.h>

#define DVM_UI_TEXT_MAX 1024U  /* Maximum payload bytes, excluding the fixed ASCII header. */
#define DVM_UI_PENDING_MAX 32U /* Buffered messages per direction; no unbounded input backlog. */
#define DVM_UI_HEADER_SIZE 53U /* Five prefix bytes and six eight-digit hexadecimal words. */
#define DVM_UI_UI 0U           /* Endpoint owned by the AS/SWF interface. */
#define DVM_UI_DIS 1U          /* Endpoint owned by the Dis service adapter. */


/* EN: Input events use id zero; requests/replies require a nonzero id.
 * RU: У событий id равен нулю; у запросов и ответов id ненулевой. */
typedef enum DVM_UIKind
{
    DVM_UI_REQUEST = 1, /* Service request; code names an agreed operation. */
    DVM_UI_REPLY = 2,   /* Correlated service reply; code names an agreed status. */
    DVM_UI_POINTER = 3, /* Pointer position in twips; code is button, flags is phase. */
    DVM_UI_KEY = 4,     /* code is virtual key, flags is pressed, first is ASCII. */
    DVM_UI_TEXT = 5,    /* Committed text from FlashVM's host input path, not key synthesis. */
    DVM_UI_FOCUS = 6,   /* flags is gained/lost focus; consumers reset held keys on loss. */
    DVM_UI_VIEW = 7     /* Dis-owned presentation notification; id is zero. */
} DVM_UIKind;


/* EN: Copied value record; no VM pointers, native handles or implicit heap ownership.
 * RU: Копия значений без указателей VM, native handles и неявного владения кучей. */
typedef struct DVM_UIMessage
{
    unsigned long kind;         /* One DVM_UIKind value. */
    unsigned long id;           /* Unsigned 32-bit correlation ID; events use zero. */
    unsigned long code;         /* Unsigned 32-bit operation, status, button or key. */
    unsigned long flags;        /* Unsigned 32-bit class-specific flags. */
    long first;                 /* Signed 32-bit first argument or pointer X in twips. */
    long second;                /* Signed 32-bit second argument or pointer Y in twips. */
    size_t size;                /* Active Canonical UTF-8 payload bytes, including any NUL. */
    char text[DVM_UI_TEXT_MAX]; /* Owned bytes; no terminator is required or appended. */
} DVM_UIMessage;


#endif
