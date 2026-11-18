/* Copyright (c) 2026 bhgv
 * EN: Owned native CLI selection, explicit delegated carrier and application-tag grants.
 * RU: Собственный native CLI-профиль, явно переданный carrier и grants клиентских тегов.
 */
#ifndef UNIBUS_CLI_CONFIG_H
#define UNIBUS_CLI_CONFIG_H
#include "order/broker/broker.h"
#include <stddef.h>
#define UNIBUS_CLI_NONE 0U /* No transport, workers or stdin/stdout processing. */
#define UNIBUS_CLI_ORDERED 1U /* Explicit P9MX carrier and negotiated ordered Unibus. */


/* EN/RU: Immutable after parse; strings copied, tags owned until clear, never serialize native layout.
 * Неизменяемые после parse настройки; строки скопированы, tags принадлежат до clear; не wire-структура.
 */
typedef struct UnibusCliConfig
{
    unsigned profile; /* NONE or ORDERED; default NONE. */
    int help; /* Print help before opening any descriptor. */
    int fd; /* Delegated reliable ordered carrier FD>=3, still caller-owned. */
    unsigned channel; /* Explicit P9MX client session0/1, default0. */
    unsigned limit; /* Inner body ceiling, default4096. */
    unsigned timeout; /* Native RPC/profile agreement budget1..120000ms, default3000. */
    unsigned long long window; /* Future per-tag displacement, default1024. */
    unsigned long lane; /* Required nonzero native outgoing outer lane. */
    size_t records; /* Optional receive quota;0 runs until transport retirement/error. */
    unsigned long *tags; /* Owned explicit unique inner grants, including optional application-local0. */
    size_t count; /* Number of grants copied from repeated --tag arguments. */
    char root[DVM_NS_PATH_LIMIT]; /* Canonical remote capture path, default dot. */
    char receive[DVM_NS_PATH_LIMIT]; /* Canonical channel relative to captured root, default records. */
    char send[DVM_NS_PATH_LIMIT]; /* Canonical outgoing channel, default records. */
} UnibusCliConfig;


/* EN: Parse borrowed argv into empty config,1 success/0 error; error names one invalid option, never contains wire data.
 * No I/O or environment lookup. Clear after every outcome; config remains internally owned/clearable on failure.
 * RU: Разобрать argv в пустой config:1/0, error называет неверный параметр без данных канала.
 * Без I/O/окружения. Clear вызывается при любом исходе, включая отказ.
 */
int unibuscli_parse(int argc, char *const *argv, UnibusCliConfig *config, const char **error);


/* EN/RU: Consume owned tag storage, reset descriptor; NULL harmless, no I/O.
 * Освободить собственные tags и сбросить descriptor; NULL безопасен, без I/O.
 */
void unibuscli_clear(UnibusCliConfig *config);


/* EN/RU: Print human-facing CLI help before binary traffic begins; returns nothing.
 * Вывести help до начала двоичного трафика; без возвращаемого значения.
 */
void unibuscli_help(void);
#endif
