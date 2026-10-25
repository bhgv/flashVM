/* Inject native allocation/mutex failure before any VM/SDL subsystem startup. */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>

static int fail_allocation, fail_mutex;


/* Only the mailbox storage allocation is redirected; SDL remains an ordinary dependency. */
static void *fault_calloc(size_t count, size_t size)
{
    return fail_allocation ? 0 : calloc(count, size);
}


/* A native mutex failure must release the prior allocation and reserved handle quota. */
static SDL_mutex *fault_mutex(void)
{
    return fail_mutex ? 0 : SDL_CreateMutex();
}

#define calloc fault_calloc
#define SDL_CreateMutex fault_mutex
#include "../common/playback_mailbox.c"

#undef SDL_CreateMutex
#undef calloc
#define CHECK(expression)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expression))                                                                                             \
        {                                                                                                              \
            fprintf(stderr, "allocation line %d\n", __LINE__);                                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


/* Verify untouched output and repeatable recovery after each independently injected failure. */
int main(void)
{
    FVM_DesktopPlaybackMailbox *box = 0, *before;
    int i;
    CHECK(fvm_desktop_playback_mailbox_create(&box) == FVM_OK);
    before = box;
    for (i = 0; i < 8; ++i)
    {
        fail_allocation = 1;
        CHECK(fvm_desktop_playback_mailbox_create(&box) == FVM_OUT_OF_MEMORY && box == before);
        CHECK(fvm_desktop_playback_mailbox_active() == 1);
        fail_allocation = 0;
        fail_mutex = 1;
        CHECK(fvm_desktop_playback_mailbox_create(&box) == FVM_ERROR && box == before);
        CHECK(fvm_desktop_playback_mailbox_active() == 1);
        fail_mutex = 0;
    }
    CHECK(fvm_desktop_playback_mailbox_release(box) == FVM_OK);
    CHECK(!fvm_desktop_playback_mailbox_active());
    CHECK(fvm_desktop_playback_mailbox_create(&box) == FVM_OK);
    CHECK(fvm_desktop_playback_mailbox_release(box) == FVM_OK);
    CHECK(!fvm_desktop_playback_mailbox_active() && !SDL_WasInit(0));
    puts("PASS native allocation/mutex rollback and unchanged output");
    return 0;
}
