/* FlashVM: Public embedding API declarations: render jobs.
 * File: include/flashvm/render_jobs.h.
 */

#ifndef FLASHVM_RENDER_JOBS_H
/* Define FLASHVM RENDER JOBS H constant or compilation switch. */
#define FLASHVM_RENDER_JOBS_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif


/* Independent drawing jobs. Callbacks must not touch the live AVM1 VM or
     * mutate shared display objects. Supply immutable snapshots and distinct outputs.
     * This is a host-side parallel rendering primitive, not a SWF renderer. */
typedef void (*FVM_RenderJob)(void *userdata, unsigned int index);
/* Returns 0 on success, -1 for invalid input or thread failure.
     * Waits for all workers before returning. workers=0 means synchronous mode.
     * Jobs may execute out of order. */


/* Render jobs run. */
int fvm_render_jobs_run(unsigned int job_count, unsigned int workers, FVM_RenderJob callback,
                        void *userdata);
#ifdef __cplusplus
}
#endif
#endif
