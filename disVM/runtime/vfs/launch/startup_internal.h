/* Copyright (c) 2026 bhgv
 * EN: Private bridge from initial frame construction to the runtime process constructor.
 * RU: Private bridge начального frame к runtime process constructor.
 */
#ifndef DVM_LAUNCH_STARTUP_INTERNAL_H
#define DVM_LAUNCH_STARTUP_INTERNAL_H
#include "runtime.h"
struct Modlink; /* Borrowed managed link retained by the caller until publication. */
struct REG; /* Borrowed private initial register/stack image. */


/* [GC:] EN/RU: ACTIVE startup controller constructs/publishes initial process; errors unwind caller.
 * Borrow link/registers/handoff; move namespace/channels only at adoption, retain module on success.
 * ACTIVE controller создаёт process; borrowed inputs, adoption переносит namespace/channels. */
struct Prog *dvm_runtime_create_initial(struct Modlink *link, const struct REG *initial, DVM_LaunchHandoff *handoff);
#endif
