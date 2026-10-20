/* Real source transfer and deferred GetURL2 replacement; no socket/device assumption. */
#define SDL_MAIN_HANDLED
#include "../common/player.h"
#include "flashvm/swf.h"
#include "../../examples/player/embedded_movie/movie.h"
#include <SDL.h>
#include <assert.h>
#include <string.h>
static const unsigned char parent_movie[] = {
    70, 87, 83,  8,   52,  0,   0,  0,   128, 0,   0,   200, 0,   0,  0,   125, 0,   0,
    24, 1,  0,   25,  3,   150, 17, 0,   0,   110, 101, 120, 116, 46, 115, 119, 102, 0,
    0,  95, 114, 111, 111, 116, 0,  154, 1,   0,   64,  0,   64,  0,  0,   0};
static const unsigned char level_movie[] = {
    70, 87, 83,  8,   54,  0,   0,   0,  128, 0,   0,   200, 0,   0,  0,   125, 0,   0,
    24, 1,  0,   27,  3,   150, 19,  0,  0,   110, 101, 120, 116, 46, 115, 119, 102, 0,
    0,  95, 108, 101, 118, 101, 108, 48, 0,   154, 1,   0,   64,  0,  64,  0,   0,   0};
static const unsigned char child_target[] = {
    70, 87, 83,  8,   52,  0,   0,  0,   128, 0,   0,   200, 0,   0,  0,   125, 0,   0,
    24, 1,  0,   25,  3,   150, 17, 0,   0,   110, 101, 120, 116, 46, 115, 119, 102, 0,
    0,  99, 104, 105, 108, 100, 0,  154, 1,   0,   64,  0,   64,  0,  0,   0};
static const unsigned char post_movie[] = {
    70, 87, 83,  8,   52,  0,   0,  0,   128, 0,   0,   200, 0,   0,  0,   125, 0,   0,
    24, 1,  0,   25,  3,   150, 17, 0,   0,   110, 101, 120, 116, 46, 115, 119, 102, 0,
    0,  95, 114, 111, 111, 116, 0,  154, 1,   0,   66,  0,   64,  0,  0,   0};

static unsigned int calls;
static int scenario;
/* Transport sees borrowed request/base, streams bytes, and returns canonical provenance. */
static FVM_Result fetch(const char *ref, const char *base, FVM_DesktopWrite write, void *sink,
                        char *resolved, size_t capacity, void *userdata)
{
    const unsigned char *data = embedded_movie;
    size_t size = sizeof(embedded_movie), i, chunk;
    const char *canonical = "mock://origin/next.swf";
    (void)userdata;
    ++calls;
    if (calls == 1)
    {
        assert(!base && !strcmp(ref, "mock://origin/parent.swf"));
        canonical = "mock://origin/parent.swf";
        data = parent_movie;
        size = sizeof(parent_movie);
        if (scenario == 5)
        {
            data = child_target;
            size = sizeof(child_target);
        }
        if (scenario == 6)
        {
            data = post_movie;
            size = sizeof(post_movie);
        }
        if (scenario == 7)
        {
            data = level_movie;
            size = sizeof(level_movie);
        }
    }
    else
    {
        assert(!strcmp(ref, "next.swf"));
        assert(!strcmp(base, calls == 2 ? "mock://origin/parent.swf" : "mock://origin/next.swf"));
    }
    if (scenario == 8)
    {
        data = parent_movie;
        size = sizeof(parent_movie);
    }
    assert(capacity > strlen(canonical));
    strcpy(resolved, canonical);
    if (scenario == 3)
        memset(resolved, 'x', capacity);
    if (scenario == 2)
        assert(write(data, (size_t)FVM_SWF_DEFAULT_LIMIT + 1, sink) != FVM_OK);
    for (i = 0; i < size; i += chunk)
    {
        FVM_Result status;
        chunk = size - i > 7 ? 7 : size - i;
        status = write(data + i, chunk, sink);
        if (scenario != 2 && status != FVM_OK)
            return status;
    }
    return scenario == 1 ? FVM_ERROR : FVM_OK;
}
/* Run both VM modes against the actual importer and shared SDL presentation. */
int main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    FVM_DesktopLoader loader;
    FVM_DesktopSource source;
    const char *error;
    assert(fvm_desktop_options_init(&options));
    assert(fvm_desktop_loader_init(&loader));
    options.mode =
        argc > 1 && !strcmp(argv[1], "--jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    if (options.mode == FVM_EXECUTION_JIT && !fvm_jit_available())
        return 77;
    options.mute = 1;
    options.frames = 2;
    options.fps = 1000;
    source.kind = FVM_DESKTOP_SOURCE_URL;
    source.bytes = 0;
    source.size = 0;
    source.location = "mock://origin/parent.swf";
    loader.fetch = fetch;
    loader.max_loads = 3;
    for (scenario = 0; scenario <= 8; ++scenario)
    {
        int status;
        calls = 0;
        status = fvm_desktop_run_source(&options, &source, &loader, &error);
        if (scenario == 0 || scenario == 7)
            assert(!status && !error && calls == 2);
        else
            assert(status == FVM_DESKTOP_EXIT_PLAYBACK && error &&
                   calls == (scenario == 8 ? 3U : 1U));
        if (scenario == 3)
            ++scenario; /* Scenario 4 reserved for malformed descriptors below. */
    }
    calls = 0;
    loader.max_loads = 0;
    assert(fvm_desktop_run_source(&options, &source, &loader, 0) == FVM_DESKTOP_EXIT_ARGUMENT);
    loader.max_loads = 2;
    source.bytes = embedded_movie;
    assert(fvm_desktop_run_source(&options, &source, &loader, 0) == FVM_DESKTOP_EXIT_ARGUMENT);
    assert(calls == 0);
    source.bytes = 0;
    loader.fetch = 0;
    assert(fvm_desktop_run_source(&options, &source, &loader, 0) == FVM_DESKTOP_EXIT_PLAYBACK);
    loader.fetch = fetch; loader.max_loads = 1; calls = 0; scenario = 0;
    assert(fvm_desktop_run_source(&options, &source, &loader, 0) == FVM_DESKTOP_EXIT_PLAYBACK);
    assert(calls == 1);
    source.kind = FVM_DESKTOP_SOURCE_MEMORY; source.bytes = embedded_movie;
    source.size = sizeof(embedded_movie); source.location = "embedded://source.swf";
    assert(fvm_desktop_run_source(&options, &source, 0, &error) == 0 && !error);
    SDL_Quit();
    return 0;
}
