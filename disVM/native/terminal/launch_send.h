/* Copyright (c) 2026 bhgv
 * EN: Route one launch request to an already authorized remote Styx node.
 * RU: Отправка одного запроса запуска в выданный удалённый узел Styx.
 */
#ifndef STYX_TERMINAL_LAUNCH_SEND_H
#define STYX_TERMINAL_LAUNCH_SEND_H
#include "launch/route.h"
#include "launch/ticket.h"

/* EN: Borrow the trusted writable, single-file provider and launch tag. The
 * returned address retains that exact file capability. The service must accept
 * a complete S-record in one write at offset zero and reject incomplete input.
 * RU: Заимствовать выданный provider одного файла и тег запуска. Адрес удерживает
 * полномочие этого файла. Служба принимает полную S-запись одной записью с
 * нулевого смещения, неполные запросы отвергает. */
DVM_LaunchAddress *styxterminallaunchaddress(DVM_Provider *request_file, unsigned long tag);
/* EN/RU: Like the raw address, with a copied selector for a configured module route.
 * Как прежний адрес, с копией выбора процесса для настроенного маршрута модуля. */
DVM_LaunchAddress *styxterminallaunchprogramaddress(DVM_Provider *request_file, unsigned long tag,
    const DVM_LaunchProgram *program);
/* EN/RU: Opt into a process-scoped launch using the runtime caller's PID.
 * Включить запуск с контекстом PID вызывающего процесса. */
DVM_LaunchAddress *styxterminallaunchcalleraddress(DVM_Provider *request_file, unsigned long tag);
#endif
