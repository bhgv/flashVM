/* Borrow stack and compressed binary input, including embedded zero bytes. */
#define SDL_MAIN_HANDLED
#include "../common/player.h"
#include "flashvm/swf.h"
#include "../../examples/player/embedded_movie/movie.h"
#include <SDL.h>
#include <assert.h>
#include <string.h>
static const unsigned char compressed[] = {67,87,83,8,44,0,0,0,120,156,107,96,96,56,193,192,192,80,203,192,32,193,200,224,204,100,16,80,224,194,205,103,199,204,112,140,5,40,42,192,240,193,1,72,1,0,111,18,5,188};
#ifdef FVM_TEST_PCM_QUEUE
/* Independent complete PCM root movie, copied to a mutable stack buffer before playback. */
static const unsigned char root_pcm[] = {70,87,83,8,44,0,0,0,128,0,0,200,0,0,0,125,0,0,24,1,0,67,2,48,80,112,68,11,14,62,3,0,198,4,0,0,0,16,0,240,64,0,0,0};
#endif
/* An intentionally invalid diagnostic location proves the buffer path does not open files. */
int main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    char buf[sizeof(embedded_movie)];
#ifdef FVM_TEST_PCM_QUEUE
    char pcm[sizeof(root_pcm)];
#endif
    const char *error = 0;
    assert(fvm_desktop_options_init(&options));
    options.mode = argc > 1 && !strcmp(argv[1], "--jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    if (options.mode == FVM_EXECUTION_JIT && !fvm_jit_available()) return 77;
    options.mute = 1; options.frames = 2; options.fps = 1000; options.stats = 1;
    memcpy(buf, embedded_movie, sizeof(buf));
    assert(fvm_desktop_run_memory(&options, 0, sizeof(buf), "memory", &error) == FVM_DESKTOP_EXIT_ARGUMENT && error);
    assert(fvm_desktop_run_memory(&options, buf, 0, "memory", 0) == FVM_DESKTOP_EXIT_ARGUMENT);
    assert(fvm_desktop_run_memory(&options, buf, (size_t)FVM_SWF_DEFAULT_LIMIT + 1, "memory", 0) == FVM_DESKTOP_EXIT_ARGUMENT);
    assert(fvm_desktop_run_memory(0, buf, sizeof(buf), "memory", 0) == FVM_DESKTOP_EXIT_ARGUMENT);
    assert(fvm_desktop_run_memory(&options, buf, sizeof(buf), "", 0) == FVM_DESKTOP_EXIT_ARGUMENT);
    assert(fvm_desktop_run_memory(&options, buf, 3, "memory", &error) == FVM_DESKTOP_EXIT_PLAYBACK && error);
    assert(fvm_desktop_run_memory(&options, buf, sizeof(buf), "not-a-file://memory", &error) == 0 && !error);
    assert(!memcmp(buf, embedded_movie, sizeof(buf)));
    assert(fvm_desktop_run_memory(&options, compressed, sizeof(compressed), "not-a-file://compressed", &error) == 0 && !error);
#ifdef FVM_TEST_PCM_QUEUE
    assert(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0);
    memcpy(pcm,root_pcm,sizeof(pcm));options.mute=0;options.fps=10;
    assert(fvm_desktop_run_memory(&options,pcm,sizeof(pcm),"not-a-file://pcm-stream",&error)==0 && !error);
    assert(!memcmp(pcm,root_pcm,sizeof(pcm)));
#endif
    SDL_Quit();
    return 0;
}
