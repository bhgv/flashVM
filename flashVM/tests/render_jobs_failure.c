/* Fault-inject thread-start failure into the production synchronous partition runner. */
#define fvm_port_thread_start test_thread_start
#define fvm_port_thread_join test_thread_join
#include "../src/render/render_jobs.c"
#include <stdio.h>

static unsigned int attempted; /* Owner-only mock start calls. */
static unsigned int fail_at;   /* Mock start call that returns failure. */
static unsigned int joined;    /* Mock handles consumed after fallback. */
static int handle;             /* Opaque mock storage, never dereferenced as a port handle. */


/* Execute accepted partitions immediately, failing exactly the selected start call. */
FVM_PortThread *test_thread_start(FVM_PortThreadFn callback, void *argument)
{
    if (attempted++ == fail_at)
    {
        return NULL;
    }
    callback(argument);
    return (FVM_PortThread *)&handle;
}


/* Record mock join consumption; the actual platform thread implementation is not entered. */
void test_thread_join(FVM_PortThread *thread)
{
    if (thread == (FVM_PortThread *)&handle)
    {
        joined++;
    }
}


/* Count each initialized task; userdata borrows the thirteen-element result array. */
static void paint(void *userdata, unsigned int index)
{
    unsigned int *counts = (unsigned int *)userdata;
    if (index < 13)
    {
        counts[index]++;
    }
}


/* Every partial launch must complete all partitions exactly once, then report failure. */
int main(void)
{
    for (fail_at = 0; fail_at < 5; fail_at++)
    {
        unsigned int counts[13] = {0};
        unsigned int index;
        attempted = 0;
        joined = 0;
        if (fvm_render_jobs_run(13, 5, paint, counts) != -1 || joined != fail_at)
        {
            fprintf(stderr, "partial thread launch status/join failure\n");
            return 1;
        }
        for (index = 0; index < 13; index++)
        {
            if (counts[index] != 1)
            {
                fprintf(stderr, "uninitialized or repeated fallback partition %u\n", index);
                return 1;
            }
        }
    }
    puts("PASS render jobs: all five launch failures complete every initialized partition once");
    return 0;
}
