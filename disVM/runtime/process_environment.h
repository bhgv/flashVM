/* Hosted Prog environments: guest error state follows the logical process. */
#ifndef DVM_PROCESS_ENVIRONMENT_H
#define DVM_PROCESS_ENVIRONMENT_H


/* Initialize embedded native error storage before publishing a new runtime Prog.
 * Parent may be NULL; copy its bounded error text, never its pointer fields.
 * No allocation/throw. Caller holds heap exclusion and supplies a runtime wrapper.
 * Namespace/descriptor groups remain outside this hosted environment subset. */
void dvm_process_environment_init(Prog *process, Prog *parent);


/* Bind this claimed Prog's embedded environment and supported guest FP state.
 * Save native host FP control/status on the current typed up for release.
 * Return zero for malformed host metadata; no allocation or frame/root changes.
 * The claim retains Prog/context lifetime through native suspension and reentry. */
int dvm_process_environment_bind(Prog *process);


/* Restore this OS owner's stable default environment BEFORE rotation/free or
 * leaving heap ownership. Save guest FP state and restore the host FP state.
 * Does not clear the logical Prog's saved error text. */
void dvm_process_environment_release(Prog *process);
#endif
