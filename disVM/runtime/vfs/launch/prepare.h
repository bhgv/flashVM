/* Copyright (c) 2026 bhgv
 * EN: Provider-side pre-init namespace preparation from two explicitly granted live roots.
 * RU: Подготовка namespace у провайдера перед init из двух явно предоставленных живых roots.
 */
#ifndef DVM_LAUNCH_PREPARE_H
#define DVM_LAUNCH_PREPARE_H
#include "../resource.h"
#define DVM_LAUNCH_TERMINAL_FIRST 1U /* Only currently negotiated initial union policy. */


/* EN/RU: Borrowed granted roots and copied identity; caller retains inputs through preparation.
 * Roots are already filtered by each owner's publication preparation; no host path lookup.
 * Заимствованные granted roots и copied identity; caller удерживает inputs до конца подготовки. */
typedef struct DVM_LaunchOptions
{
    unsigned policy; /* Explicit negotiated policy; unknown values refuse without side effects. */
    DVM_Provider *terminal; /* Granted live terminal root, accessed through a transport proxy when remote. */
    DVM_Provider *provider; /* Granted live execution-provider root, not its whole host filesystem. */
    DVM_Origin terminal_origin; /* Trusted terminal association; role must match. */
    DVM_Origin provider_origin; /* Trusted provider association, same launch session. */
} DVM_LaunchOptions;


/* EN/RU: Validate bounded copied source association and expected role, without I/O/authentication.
 * Borrow origin; return1/0. Проверить origin/role без I/O/authentication, вернуть1/0. */
int dvm_launch_origin_valid(const DVM_Origin *origin, unsigned role);


/* EN/RU: Build owned isolated namespace/NULL before init. Native-only metadata/capture may block;
 * caller supplies a scope only for capable roots. No guest execution, FD transfer or wire protocol.
 * Capture roots, validate directories and cancel, then publish privately built terminal-first stack.
 * Создать owned isolated namespace/NULL до init; native metadata/capture может ждать.
 * Без guest execution/FD transfer/wire; roots удержаны, dirs/cancel проверены до выдачи результата. */
DVM_Namespace *dvm_launch_prepare(const DVM_LaunchOptions *options);
#endif
