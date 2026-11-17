/* Copyright (c) 2026 bhgv
 * EN: Shared allocation-free IDE fvd1 byte codec; no channel, VM, graphics or transport operations.
 * RU: Общий allocation-free IDE fvd1 byte codec без channel/VM/graphics/transport operations.
 */
#include "disvm/ui_frame.h"
#include <string.h>


/* Admit canonical UTF-8 Unicode scalars so AS and Limbo cannot disagree through replacement.
 * text borrows size bytes, including NUL. Returns one for valid encoding, otherwise zero. */
/* RU: Проверить canonical UTF-8 в borrowed text/size без side effects;1/0. */
static int valid_text(const char *text, size_t size)
{
    size_t i = 0; /* Next borrowed UTF-8 byte. */
    while (i < size)
    {
        unsigned long scalar; /* Accumulated Unicode scalar. */
        unsigned long minimum; /* Smallest scalar allowed by this sequence length. */
        unsigned int count; /* Remaining continuation bytes. */
        unsigned int c = (unsigned char)text[i++]; /* Current unsigned byte. */
        if (c < 128)
        {
            continue;
        }
        if (c >= 194 && c <= 223)
        {
            scalar = c & 31U;
            minimum = 128;
            count = 1;
        }
        else if (c >= 224 && c <= 239)
        {
            scalar = c & 15U;
            minimum = 2048;
            count = 2;
        }
        else if (c >= 240 && c <= 244)
        {
            scalar = c & 7U;
            minimum = 65536;
            count = 3;
        }
        else
        {
            return 0;
        }
        if (size - i < count)
        {
            return 0;
        }
        while (count)
        {
            c = (unsigned char)text[i++];
            if ((c & 192U) != 128U)
            {
                return 0;
            }
            scalar = (scalar << 6) | (c & 63U);
            count--;
        }
        if (scalar < minimum || scalar > 0x10ffffUL || (scalar >= 0xd800UL && scalar <= 0xdfffUL))
        {
            return 0;
        }
    }
    return 1;
}


/* Validate portable scalar ranges before encoding or accepting an inbound record.
 * m is borrowed and nonnull. Returns one for an admitted common envelope, otherwise zero.
 * Operation-specific permission/schema checks remain with the receiving service. */
/* RU: Проверить borrowed message scalars/payload,1/0, без side effects. */
static int valid_message(const DVM_UIMessage *m)
{
    if (m->kind < DVM_UI_REQUEST || m->kind > DVM_UI_VIEW || m->id > 0xffffffffUL || m->code > 0xffffffffUL ||
        m->flags > 0xffffffffUL || m->first < (-2147483647L - 1L) || m->first > 2147483647L ||
        m->second < (-2147483647L - 1L) || m->second > 2147483647L || m->size > DVM_UI_TEXT_MAX)
    {
        return 0;
    }
    if (!valid_text(m->text, m->size))
    {
        return 0;
    }
    if (m->kind == DVM_UI_REQUEST || m->kind == DVM_UI_REPLY)
    {
        return m->id != 0;
    }
    return m->id == 0;
}


/* Write one unsigned 32-bit value as eight lowercase ASCII hexadecimal digits.
 * p points to eight writable bytes; value has already been range checked. Returns nothing. */
/* RU: Записать value как восемь ASCII hex digits в borrowed p; значение проверено. */
static void put_word(char *p, unsigned long value)
{
    static const char digits[] = "0123456789abcdef"; /* Locale-independent wire alphabet. */
    unsigned int i; /* Hexadecimal digit position. */
    for (i = 0; i < 8; i++)
    {
        p[7 - i] = digits[value & 15UL];
        value >>= 4;
    }
}


/* Parse eight lowercase hexadecimal digits. p borrows eight bytes, out receives a word.
 * Returns zero on malformed input; out is only written on success. */
/* RU: Прочитать восемь digits из borrowed p; out меняется только при успехе,1/0. */
static int get_word(const char *p, unsigned long *out)
{
    unsigned int i; /* Hexadecimal digit position. */
    unsigned long value = 0; /* Parsed unsigned 32-bit word. */
    for (i = 0; i < 8; i++)
    {
        unsigned int digit; /* Admitted lowercase hexadecimal value. */
        if (p[i] >= '0' && p[i] <= '9')
        {
            digit = (unsigned int)(p[i] - '0');
        }
        else if (p[i] >= 'a' && p[i] <= 'f')
        {
            digit = (unsigned int)(p[i] - 'a' + 10);
        }
        else
        {
            return 0;
        }
        value = (value << 4) | digit;
    }
    *out = value;
    return 1;
}


/* Reconstruct an int32 value without implementation-defined unsigned-to-signed narrowing.
 * value is a parsed 32-bit word. Returns its signed two's-complement mathematical value. */
/* RU: Преобразовать unsigned int32 bits в математическое signed значение. */
static long signed_word(unsigned long value)
{
    if (value <= 2147483647UL)
    {
        return (long)value;
    }
    return -1L - (long)(0xffffffffUL - value);
}


/* EN/RU: See ui_frame.h; validate all fields before copying the portable frame.
 * См. ui_frame.h; проверить поля до копирования portable frame. */
FVM_Result dvm_ui_frame_encode(const DVM_UIMessage *message, char *storage, size_t capacity, size_t *used)
{
    if (!message || !storage || !used || !valid_message(message) || capacity < DVM_UI_HEADER_SIZE + message->size)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memcpy(storage, "fvd1:", 5);
    put_word(storage + 5, message->kind);
    put_word(storage + 13, message->id);
    put_word(storage + 21, message->code);
    put_word(storage + 29, message->flags);
    put_word(storage + 37, ((unsigned long)message->first) & 0xffffffffUL);
    put_word(storage + 45, ((unsigned long)message->second) & 0xffffffffUL);
    memcpy(storage + DVM_UI_HEADER_SIZE, message->text, message->size);
    *used = DVM_UI_HEADER_SIZE + message->size;
    return FVM_OK;
}


/* EN/RU: See ui_frame.h; decode into private values and publish only after full validation.
 * См. ui_frame.h; выдача только после полной проверки native values. */
FVM_Result dvm_ui_frame_decode(const void *bytes, size_t length, DVM_UIMessage *out)
{
    DVM_UIMessage decoded; /* Private copy, published only on complete validation. */
    unsigned long first; /* First signed argument as wire bits. */
    unsigned long second; /* Second signed argument as wire bits. */
    const char *text = (const char *)bytes; /* Borrowed counted frame. */
    if (!bytes || !out || length < DVM_UI_HEADER_SIZE || length > DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX ||
        memcmp(text, "fvd1:", 5))
    {
        return FVM_INVALID_ARGUMENT;
    }
    memset(&decoded, 0, sizeof(decoded));
    if (!get_word(text + 5, &decoded.kind) || !get_word(text + 13, &decoded.id) ||
        !get_word(text + 21, &decoded.code) || !get_word(text + 29, &decoded.flags) ||
        !get_word(text + 37, &first) || !get_word(text + 45, &second))
    {
        return FVM_INVALID_ARGUMENT;
    }
    decoded.first = signed_word(first);
    decoded.second = signed_word(second);
    decoded.size = length - DVM_UI_HEADER_SIZE;
    memcpy(decoded.text, text + DVM_UI_HEADER_SIZE, decoded.size);
    if (!valid_message(&decoded))
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = decoded;
    return FVM_OK;
}


