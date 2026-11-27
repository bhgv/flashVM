/* Copyright (c) 2026 bhgv
 * EN: Initialize the native Inferno server's reused request/response Fcall.
 * RU: Инициализация общего Fcall запросов и ответов нативного сервера Inferno.
 */
#include <lib9.h>
#include <styx.h>


/* EN: Decode a borrowed packet of size bytes into frame after clearing
 * response-only fields, including the default Ropen and Rcreate iounit.
 * Return the original codec result. The caller owns frame.
 * RU: Декодировать заимствованный пакет из size байтов в frame после
 * обнуления полей ответа, включая iounit в Ropen и Rcreate. Вернуть
 * результат исходного декодера. Владение frame остаётся у вызывающего. */
uint styx_hosted_request_decode(uchar *bytes, uint size, Fcall *frame)
{
    memset(frame, 0, sizeof(*frame));
    return convM2S(bytes, size, frame);
}
