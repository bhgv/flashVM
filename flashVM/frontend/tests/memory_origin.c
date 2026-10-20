/* Simulate OS executable-directory discovery where sandbox /proc is unavailable. */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
static const char *executable_directory;
/* The actual SDL discovery call is isolated; resource resolution remains production code. */
static char *test_base_path(void)
{
    return SDL_strdup(executable_directory);
}
#define FVM_DESKTOP_EXECUTABLE_BASE test_base_path
#include "../common/player.c"
#undef FVM_DESKTOP_EXECUTABLE_BASE
/* Remove the source file before playing its char buffer from a different working directory. */
int main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    unsigned char *bytes;
    size_t size = 0;
    int status;
    if (argc != 4)
        return 2;
    executable_directory = argv[2];
    bytes = read_movie(argv[1], &size);
    if (!bytes || remove(argv[1]))
        return 1;
    fvm_desktop_options_init(&options);
    options.mode = !strcmp(argv[3], "--jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    options.mute = 1;
    options.max_loads = 8;
    options.frames = 3;
    options.fps = 1000;
    status = fvm_desktop_run_memory(&options, bytes, size, "embedded://startup.swf", 0);
    free(bytes);
    SDL_Quit();
    return status;
}
