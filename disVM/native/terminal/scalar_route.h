/* Copyright (c) 2026 bhgv
 * EN: Route scalar calls and replies over preauthorized Unibus file capabilities.
 * RU: Маршрутизация скалярных вызовов и ответов через выданные файлы Unibus.
 */
#ifndef STYX_SCALAR_ROUTE_H
#define STYX_SCALAR_ROUTE_H
#include "scalar_record.h"
#include "launch_send.h"


/* EN/RU: Send one request on a fresh write fid; borrow the exact peer file.
 * Отправить один запрос через новый fid записи; файл другой стороны заимствован. */
DVM_LaunchDelivery styxscalarcallfile(DVM_Provider *peer, unsigned long tag, int64_t argument);


/* EN/RU: Execute one validated call and write its reply to a separate peer file.
 * Return value describes reply delivery; the method can have run after a failed reply.
 * Выполнить проверенный вызов и записать ответ в отдельный файл другой стороны.
 * Результат описывает доставку ответа; метод мог выполниться при ошибке доставки. */
DVM_LaunchDelivery styxscalarreplyfile(DVM_Provider *peer, unsigned long call_tag,
    const void *request, size_t size, unsigned long reply_tag,
    int (*method)(void *, int64_t, int64_t *), void *state);

/* EN/RU: Write one orderly end marker on the exact request file.
 * Записать признак завершения в точный файл запросов. */
DVM_LaunchDelivery styxscalarclosefile(DVM_Provider *peer, unsigned long tag);

/* EN/RU: Recognize an exact tagged end marker.
 * Распознать точный признак завершения с тегом. */
int styxscalarclosed(unsigned long tag, const void *bytes, size_t size);
#endif
