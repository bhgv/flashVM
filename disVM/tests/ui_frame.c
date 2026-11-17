/* Copyright (c) 2026 bhgv
 * EN: Independent fvd1 golden bytes, rejection atomicity and counted UTF-8 bounds.
 * RU: Независимые эталонные байты fvd1, атомарность отказа и границы UTF-8.
 */
#include "disvm/ui_frame.h"
#include <stdio.h>
#include <string.h>


/* EN: Test pure encode/decode against fixed bytes, without a VM, queues or I/O threads.
 * Return 0 on success, 1 with stderr on the first contract failure; no owned heap storage.
 * RU: Проверить encode/decode по фиксированным байтам без VM, очередей и потоков I/O.
 * Вернуть 0 при успехе, 1 с диагностикой при ошибке; выделений памяти нет.
 */
int main(void)
{
    static const char golden[] =
        "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
    DVM_UIMessage message; /* Caller-owned values including embedded NUL and Unicode. */
    DVM_UIMessage output; /* Last accepted output or failure sentinel. */
    DVM_UIMessage before; /* Byte-for-byte output sentinel. */
    char frame[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX]; /* Fixed bounded encoding buffer. */
    size_t used = 9999; /* Output length sentinel. */
    size_t i; /* Exhaustive truncation length. */

    memset(&message, 0, sizeof(message));
    message.kind = DVM_UI_REQUEST;
    message.id = 42;
    message.code = 77;
    message.first = -2147483647L - 1L;
    message.second = 2147483647L;
    message.size = 4;
    memcpy(message.text, "A\0\320\257", message.size);
    if (dvm_ui_frame_encode(&message, frame, sizeof(frame), &used) != FVM_OK ||
        used != sizeof(golden) - 1 || memcmp(frame, golden, used))
    {
        fputs("fvd1: independent golden encoding mismatch\n", stderr);
        return 1;
    }
    if (dvm_ui_frame_decode(golden, sizeof(golden) - 1, &output) != FVM_OK ||
        memcmp(&message, &output, sizeof(message)))
    {
        fputs("fvd1: independent counted golden decoding mismatch\n", stderr);
        return 1;
    }

    memset(&output, 0x5a, sizeof(output));
    before = output;
    for (i = 0; i < DVM_UI_HEADER_SIZE; i++)
    {
        if (dvm_ui_frame_decode(golden, i, &output) != FVM_INVALID_ARGUMENT ||
            memcmp(&before, &output, sizeof(output)))
        {
            fputs("fvd1: truncated header changed caller output\n", stderr);
            return 1;
        }
    }
    if (dvm_ui_frame_decode(golden, sizeof(golden) - 2, &output) != FVM_INVALID_ARGUMENT ||
        memcmp(&before, &output, sizeof(output)))
    {
        fputs("fvd1: incomplete UTF-8 scalar changed caller output\n", stderr);
        return 1;
    }
    used = 9999;
    memset(frame, 0x5a, sizeof(frame));
    if (dvm_ui_frame_encode(&message, frame, DVM_UI_HEADER_SIZE, &used) != FVM_INVALID_ARGUMENT ||
        used != 9999 || (unsigned char)frame[0] != 0x5a)
    {
        fputs("fvd1: small output buffer was partially published\n", stderr);
        return 1;
    }

    memset(message.text, 'x', sizeof(message.text));
    message.size = sizeof(message.text);
    if (dvm_ui_frame_encode(&message, frame, sizeof(frame), &used) != FVM_OK ||
        used != sizeof(frame) || dvm_ui_frame_decode(frame, used, &output) != FVM_OK ||
        memcmp(&message, &output, sizeof(message)))
    {
        fputs("fvd1: maximum payload did not round trip\n", stderr);
        return 1;
    }
    frame[5] = 'A';
    before = output;
    if (dvm_ui_frame_decode(frame, used, &output) != FVM_INVALID_ARGUMENT ||
        memcmp(&before, &output, sizeof(output)))
    {
        fputs("fvd1: noncanonical hex accepted or output modified\n", stderr);
        return 1;
    }
    message.kind = DVM_UI_TEXT;
    if (dvm_ui_frame_encode(&message, frame, sizeof(frame), &used) != FVM_INVALID_ARGUMENT)
    {
        fputs("fvd1: event with a request ID accepted\n", stderr);
        return 1;
    }
    puts("PASS standalone fvd1 golden bytes and atomic rejection");
    return 0;
}
