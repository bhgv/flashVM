/* Native-job tutorial: joined partitions versus a nonblocking persistent queue. */
#include "flashvm/render_jobs.h" /* Joined batch API; it waits for all partitions. */
#include "flashvm/render_queue.h" /* Asynchronous native snapshots; never pass live VM objects. */
#include <stdio.h> /* Report checked results to the terminal. */
#include <stdlib.h> /* Own each queued snapshot independently of the caller stack. */

#define PIXELS 64U /* Fixed bounded demonstration output, not a full SWF display list. */


/* Paint one exclusive pixel; buffer is borrowed, pixel is a validated batch index.
 * No allocation, VM, font, window or shared display-object mutation occurs. */
static void draw(void *buffer, unsigned int pixel) /* Called on an owner or native worker. */
{ /* Begin the exclusive pixel update. */
    unsigned char *rgba = (unsigned char *)buffer; /* Borrow this job's RGBA storage. */
    rgba[pixel * 4] = 255; /* Store red independently of all other pixels. */
    rgba[pixel * 4 + 1] = (unsigned char)pixel; /* Encode the bounded pixel index in green. */
    rgba[pixel * 4 + 2] = 0; /* Store zero blue. */
    rgba[pixel * 4 + 3] = 255; /* Store opaque alpha. */
} /* The batch retains ownership of the buffer. */


/* Render one complete privately owned snapshot; value borrows its PIXELS*4 bytes.
 * Return OK after writing every pixel. No nested thread launch or interpreter access. */
static FVM_Result render(void *value) /* Queue workers invoke this native callback. */
{ /* Paint in this snapshot's independent destination. */
    unsigned int pixel; /* Traverse the bounded local image. */
    for (pixel = 0; pixel < PIXELS; pixel++) /* No other job owns these pixels. */
    { /* Begin one pixel operation. */
        draw(value, pixel); /* Reuse the same deterministic pixel computation. */
    } /* Finish the private image. */
    return FVM_OK; /* Publish completion through the queue's mutex. */
} /* The queue still owns value until take or release. */


/* Release one exclusive native allocation; accepts null, touches no VM/SDL state.
 * This callback may run on the owner or a worker for stale/cancelled snapshots. */
static void release(void *value) /* Reclaim a discarded or consumed snapshot. */
{ /* Begin native-only cleanup. */
    free(value); /* Release the independent RGBA allocation exactly once. */
} /* No pointer to the retired image is retained. */


/* Demonstrate explicit ownership transfer and cleanup; return nonzero on API failure.
 * The final wait is an intentional tutorial/output barrier, not the interactive path. */
int main(void) /* Run without a window, font or deployed scripting dependency. */
{ /* Start the sole host owner. */
    unsigned char pixels[PIXELS * 4]; /* Joined batches may safely borrow stack storage. */
    FVM_RenderQueue *queue = NULL; /* Own the persistent worker until destroy. */
    void *pending = NULL; /* Caller-owned allocation before successful submit. */
    void *completed = NULL; /* Caller-owned allocation only after successful take. */
    FVM_Result rendered = FVM_ERROR; /* Receive the worker's actual native status. */
    int result = 1; /* Report failure unless every operation is checked. */
    if (fvm_render_jobs_run(PIXELS, 4, draw, pixels) != 0) /* This call waits for all four partitions. */
    { /* Leave immediately if batch launch failed. */
        return 1; /* No queued allocation exists yet. */
    } /* Stack output is now complete. */
    if (fvm_render_queue_create(1, render, release, &queue) != FVM_OK) /* Start one persistent background worker. */
    { /* Handle allocation or OS thread failure. */
        return 1; /* Creation rolls back partial worker ownership. */
    } /* Queue now belongs to this owner. */
    pending = malloc(PIXELS * 4); /* Prepare independent storage that outlives this stack operation. */
    if (!pending || fvm_render_queue_submit(queue, pending) != FVM_OK) /* Admission never waits for raster work. */
    { /* Retain pending ownership when admission fails. */
        goto done; /* Centralize queue and allocation cleanup. */
    } /* Successful submit transfers pending to the queue. */
    pending = NULL; /* Prevent owner cleanup from freeing a worker-owned image. */
    puts("Owner continues after submit; native raster may still be running."); /* Do independent host/VM work here. */
    if (fvm_render_queue_take(queue, &completed, &rendered, 1) != FVM_OK || rendered != FVM_OK) /* Output barrier. */
    { /* Interactive loops use wait=0 and continue on NOT_FOUND instead. */
        goto done; /* Never inspect unfinished or failed pixels. */
    } /* Completion transfers native storage back to this owner. */
    if (((unsigned char *)completed)[0] != pixels[0]) /* Check the known red output independently. */
    { /* A mismatch is a real rendering failure. */
        goto done; /* Release the consumed allocation below. */
    } /* Both paths produced the expected red component. */
    puts("PASS joined batch and asynchronous native snapshot"); /* Report the verified demonstration. */
    result = 0; /* Mark all checked operations successful. */
done: /* All exit paths release exactly their current owners. */
    fvm_render_queue_destroy(queue); /* Join workers before releasing callback dependencies. */
    release(pending); /* Nonnull only if submit never accepted this allocation. */
    release(completed); /* Nonnull only after take transferred it back. */
    return result; /* Return the checked tutorial outcome. */
} /* No worker or native output survives program exit. */
