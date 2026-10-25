/* Native mailbox validation, copied-text ownership and real concurrent post/poll publication. */
#define SDL_MAIN_HANDLED
#include "../common/player.h"
#include "../common/playback_mailbox_internal.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expression)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expression))                                                                                             \
        {                                                                                                              \
            fprintf(stderr, "mailbox line %d: %s\n", __LINE__, #expression);                                           \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)
#define STRESS_READERS 4
#define STRESS_OBSERVATIONS 5000UL


/* Native scalar synchronization only; there is no VM or window in this process. */
typedef struct Stress
{
    FVM_DesktopPlaybackMailbox *mailbox;
    SDL_atomic_t done, reads, failed;
} Stress;


/* Poll consistent copied records while posting intents from real OS threads. */
static int reader(void *raw)
{
    Stress *stress = (Stress *)raw;
    FVM_DesktopPlaybackSnapshot snapshot;
    unsigned long expected;
    unsigned int turns = 0;
    FVM_Result result;
    fvm_desktop_playback_snapshot_init(&snapshot);
    while (!SDL_AtomicGet(&stress->done) || turns < 2000)
    {
        result = fvm_desktop_playback_mailbox_poll(stress->mailbox, &snapshot);
        if (result == FVM_OK)
        {
            expected = 0;
            if (snapshot.movie_ticks != snapshot.total_ticks || snapshot.presentations != snapshot.total_ticks ||
                snapshot.elapsed_ms != (double)snapshot.total_ticks ||
                snapshot.paused != (int)(snapshot.total_ticks & 1UL) ||
                sscanf(snapshot.source, "source-%lu", &expected) != 1 || expected != snapshot.total_ticks)
            {
                SDL_AtomicSet(&stress->failed, 1);
            }
            SDL_AtomicAdd(&stress->reads, 1);
        }
        else if (result != FVM_NOT_FOUND)
        {
            SDL_AtomicSet(&stress->failed, 1);
        }
        if (fvm_desktop_playback_mailbox_post(stress->mailbox, FVM_DESKTOP_PLAYBACK_STEP) != FVM_OK)
        {
            SDL_AtomicSet(&stress->failed, 1);
        }
        ++turns;
        if (!(turns & 31U))
        {
            SDL_Delay(0);
        }
    }
    return 0;
}


/* Verify exact ABI validation, terminal precedence, quota rollback and process teardown ownership. */
int main(void)
{
    FVM_DesktopPlaybackMailbox *mailbox = 0, *unchanged;
    FVM_DesktopPlaybackMailbox *quota[FVM_DESKTOP_PLAYBACK_MAILBOX_MAX_HANDLES];
    FVM_DesktopPlaybackSnapshot snapshot, before;
    FVM_DesktopPlaybackState state;
    Stress stress;
    SDL_Thread *threads[STRESS_READERS];
    unsigned long tick;
    unsigned int i;
    int status;
    char source[FVM_DESKTOP_MAX_LOCATION + 2];
    CHECK(!SDL_WasInit(0) && !fvm_desktop_playback_mailbox_active());
    CHECK(!fvm_desktop_playback_snapshot_init(0));
    CHECK(fvm_desktop_playback_snapshot_init(&snapshot));
    CHECK(fvm_desktop_playback_mailbox_create(0) == FVM_INVALID_ARGUMENT);
    CHECK(fvm_desktop_playback_mailbox_release(0) == FVM_OK);
    CHECK(fvm_desktop_playback_mailbox_stop_requested(0));
    CHECK(fvm_desktop_playback_mailbox_create(&mailbox) == FVM_OK);
    CHECK(fvm_desktop_playback_mailbox_active() == 1 && !SDL_WasInit(0));
    CHECK(fvm_desktop_shutdown() == FVM_ERROR);
    memcpy(&before, &snapshot, sizeof(snapshot));
    CHECK(fvm_desktop_playback_mailbox_poll(mailbox, &snapshot) == FVM_NOT_FOUND);
    CHECK(!memcmp(&before, &snapshot, sizeof(snapshot)));
    CHECK(fvm_desktop_playback_mailbox_post(mailbox, (FVM_DesktopPlaybackCommand)-1) == FVM_INVALID_ARGUMENT);
    CHECK(fvm_desktop_playback_mailbox_post(mailbox, FVM_DESKTOP_PLAYBACK_PAUSE) == FVM_OK);
    CHECK(fvm_desktop_playback_mailbox_post(mailbox, FVM_DESKTOP_PLAYBACK_RESUME) == FVM_OK);
    CHECK(fvm_desktop_playback_mailbox_post(mailbox, FVM_DESKTOP_PLAYBACK_KEEP) == FVM_OK);
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    state.phase = FVM_DESKTOP_PLAYBACK_READY;
    strcpy(source, "original");
    state.source = source;
    CHECK(fvm_desktop_playback_mailbox_publish(mailbox, &state) == FVM_DESKTOP_PLAYBACK_RESUME);
    strcpy(source, "changed");
    CHECK(fvm_desktop_playback_mailbox_poll(mailbox, &snapshot) == FVM_OK);
    CHECK(!strcmp(snapshot.source, "original") && snapshot.pending == FVM_DESKTOP_PLAYBACK_KEEP);
    CHECK(fvm_desktop_playback_mailbox_publish(mailbox, &state) == FVM_DESKTOP_PLAYBACK_KEEP);
    --snapshot.struct_size;
    memcpy(&before, &snapshot, sizeof(snapshot));
    CHECK(fvm_desktop_playback_mailbox_poll(mailbox, &snapshot) == FVM_INVALID_ARGUMENT);
    CHECK(!memcmp(&before, &snapshot, sizeof(snapshot)));
    fvm_desktop_playback_snapshot_init(&snapshot);
    memset(source, 'a', sizeof(source));
    source[FVM_DESKTOP_MAX_LOCATION] = 0;
    CHECK(fvm_desktop_playback_mailbox_publish(mailbox, &state) == FVM_DESKTOP_PLAYBACK_KEEP);
    CHECK(fvm_desktop_playback_mailbox_poll(mailbox, &snapshot) == FVM_OK);
    CHECK(strlen(snapshot.source) == FVM_DESKTOP_MAX_LOCATION);
    source[FVM_DESKTOP_MAX_LOCATION] = 'a';
    source[FVM_DESKTOP_MAX_LOCATION + 1] = 0;
    CHECK(fvm_desktop_playback_mailbox_publish(mailbox, &state) == FVM_DESKTOP_PLAYBACK_STOP);
    CHECK(fvm_desktop_playback_mailbox_post(mailbox, FVM_DESKTOP_PLAYBACK_STEP) == FVM_OK);
    CHECK(fvm_desktop_playback_mailbox_post(mailbox, FVM_DESKTOP_PLAYBACK_STOP) == FVM_OK);
    CHECK(fvm_desktop_playback_mailbox_post(mailbox, FVM_DESKTOP_PLAYBACK_RESUME) == FVM_ERROR);
    CHECK(fvm_desktop_playback_mailbox_post(mailbox, FVM_DESKTOP_PLAYBACK_STOP) == FVM_OK);
    CHECK(fvm_desktop_playback_mailbox_stop_requested(mailbox));
    strcpy(source, "terminal");
    CHECK(fvm_desktop_playback_mailbox_publish(mailbox, &state) == FVM_DESKTOP_PLAYBACK_STOP);
    CHECK(fvm_desktop_playback_mailbox_publish(mailbox, &state) == FVM_DESKTOP_PLAYBACK_STOP);
    CHECK(fvm_desktop_playback_mailbox_poll(mailbox, &snapshot) == FVM_OK);
    CHECK(snapshot.stop_requested && snapshot.pending == FVM_DESKTOP_PLAYBACK_STOP);
    CHECK(fvm_desktop_playback_mailbox_release(mailbox) == FVM_OK);
    CHECK(!strcmp(snapshot.source, "terminal"));
    for (i = 0; i < FVM_DESKTOP_PLAYBACK_MAILBOX_MAX_HANDLES; ++i)
    {
        CHECK(fvm_desktop_playback_mailbox_create(&quota[i]) == FVM_OK);
    }
    unchanged = quota[0];
    CHECK(fvm_desktop_playback_mailbox_create(&unchanged) == FVM_ERROR && unchanged == quota[0]);
    for (i = 0; i < FVM_DESKTOP_PLAYBACK_MAILBOX_MAX_HANDLES; ++i)
    {
        CHECK(fvm_desktop_playback_mailbox_release(quota[i]) == FVM_OK);
    }
    memset(&stress, 0, sizeof(stress));
    CHECK(fvm_desktop_playback_mailbox_create(&stress.mailbox) == FVM_OK);
    for (i = 0; i < STRESS_READERS; ++i)
    {
        threads[i] = SDL_CreateThread(reader, "mailbox reader", &stress);
        CHECK(threads[i]);
    }
    for (tick = 1; tick <= STRESS_OBSERVATIONS; ++tick)
    {
        sprintf(source, "source-%lu", tick);
        state.movie_ticks = state.total_ticks = state.presentations = tick;
        state.elapsed_ms = (double)tick;
        state.paused = (int)(tick & 1UL);
        CHECK(fvm_desktop_playback_mailbox_publish(stress.mailbox, &state) != FVM_DESKTOP_PLAYBACK_STOP);
    }
    SDL_AtomicSet(&stress.done, 1);
    for (i = 0; i < STRESS_READERS; ++i)
    {
        SDL_WaitThread(threads[i], &status);
        CHECK(!status);
    }
    CHECK(!SDL_AtomicGet(&stress.failed) && SDL_AtomicGet(&stress.reads) > 0);
    CHECK(fvm_desktop_playback_mailbox_release(stress.mailbox) == FVM_OK);
    CHECK(!fvm_desktop_playback_mailbox_active() && fvm_desktop_shutdown() == FVM_OK);
    puts("PASS mailbox latest intent, terminal stop, copied snapshots, quota and four native readers");
    return 0;
}
