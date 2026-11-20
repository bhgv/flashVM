/* Copyright (c) 2026 bhgv
 * EN: Internal one-write acknowledgement shared by native Unibus senders.
 * RU: Внутреннее подтверждение одной записи для отправителей Unibus.
 */
#ifndef STYX_TERMINAL_RECORD_WRITE_H
#define STYX_TERMINAL_RECORD_WRITE_H
#include "launch_send.h"


/* EN/RU: Borrow an exact file and owned frame through one open, write and close.
 * Заимствовать точный файл и кадр на время открытия, записи и закрытия. */
DVM_LaunchDelivery styxterminalrecordwrite(DVM_Provider *provider, const void *bytes, size_t size);

#endif
