/* Bounded native raster queue: live VM objects must never enter callbacks. */
#ifndef FLASHVM_RENDER_QUEUE_H
#define FLASHVM_RENDER_QUEUE_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define FVM_RENDER_QUEUE_MAX_WORKERS 8U /* Upper bound on simultaneously executing native jobs. */


typedef struct FVM_RenderQueue FVM_RenderQueue; /* Opaque queue; all public calls have one host owner. */


/* Render one independently owned immutable native snapshot into its private output.
 * job is borrowed for this call; return its rendering status. May run concurrently
 * for different jobs. No VM/GC, font handles, SDL, queue reentry or shared mutation. */
typedef FVM_Result (*FVM_RenderExecute)(void *job);


/* Release an exclusively owned native snapshot/output, including failed or stale jobs.
 * Runs on the owner or a worker, outside the queue lock. Must not access VM/SDL,
 * reenter the queue, throw, or retain job. Every accepted job is released once,
 * unless take transfers it back to the owner. */
typedef void (*FVM_RenderRelease)(void *job);


/* Create 1..MAX_WORKERS persistent OS workers. out changes only on success.
 * execute/release remain valid through destroy. Return INVALID_ARGUMENT, OOM or
 * ERROR on thread/port creation failure; partial workers are joined on failure. */
FVM_API FVM_Result fvm_render_queue_create(unsigned int workers, FVM_RenderExecute execute,
                                           FVM_RenderRelease release, FVM_RenderQueue **out);


/* Transfer job on success; failure leaves it with the caller. Never waits for
 * rasterization. Replace/release the pending and ready jobs; executing jobs finish
 * but stale results are released. At most workers+2 accepted jobs remain owned.
 * Owner-only; job must be nonnull, distinct, complete and independent of both VMs. */
FVM_API FVM_Result fvm_render_queue_submit(FVM_RenderQueue *queue, void *job);


/* Transfer the latest ready job and its execute result to the owner. Outputs
 * change only on OK. NOT_FOUND means no ready job; wait=1 waits for the latest
 * accepted result (for explicit capture/tests only), wait=0 never waits for raster.
 * Owner-only; waiting with no outstanding job returns NOT_FOUND immediately. */
FVM_API FVM_Result fvm_render_queue_take(FVM_RenderQueue *queue, void **job,
                                         FVM_Result *rendered, int wait);


/* Stop admission, release pending/ready work and join executing callbacks before
 * freeing the queue. Null accepted. Owner-only, may block until native jobs finish;
 * callbacks must terminate. All callback dependencies must outlive this call. */
FVM_API void fvm_render_queue_destroy(FVM_RenderQueue *queue);
#ifdef __cplusplus
}
#endif
#endif
