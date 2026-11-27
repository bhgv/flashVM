/* Copyright (c) 2026 bhgv
 * EN: Initialize hosted server Fcalls before decoding into them.
 * RU: Инициализация Fcall хостового сервера перед декодированием.
 */
#include <lib9.h>
#include <styx.h>


/* EN: Decode a borrowed request of size bytes into frame. The original
 * server reuses frame for its response, so unset response fields must
 * have deterministic defaults. Ropen and Rcreate leave iounit zero to
 * use the session's negotiated limit. Return the original codec result.
 * RU: Декодировать заимствованный запрос из size байтов в frame. Сервер
 * использует frame для ответа, поэтому неустановленные поля получают
 * определённые значения. Нулевой iounit в Ropen и Rcreate оставляет
 * согласованный предел сессии. Вернуть результат исходного кодека. */
uint styx_hosted_request_decode(uchar *bytes, uint size, Fcall *frame)
{
    memset(frame, 0, sizeof(*frame));
    return convM2S(bytes, size, frame);
}
