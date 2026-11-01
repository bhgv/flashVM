/* Owner-local private geometry brokers; never native-thread access to Dis heaps. */
#ifndef DVM_EDITOR_BULK_H
#define DVM_EDITOR_BULK_H
struct Channel; /* Borrowed typed Dis byte-array channel. */


/* Register slot1..8 or detach it with nil. Channel stays rooted by its Limbo
 * sender until drain/detach. Return0/-1; owner/TLS protection is required. */
int dvm_editor_bulk_attach(int slot, struct Channel *output);


/* Visit each independent broker once, delivering at most one copied geometry
 * record per slot before Dis resumes. Return progress0/1 or terminal failure-1.
 * Ordinary input/control routing must run first on the same heap owner. */
int dvm_editor_bulk_poll(void);


/* Abort routing and release native broker resources after all Dis owners stop.
 * Partial construction is allowed; no borrowed channel is dereferenced. */
void dvm_editor_bulk_stop(void);
#endif
