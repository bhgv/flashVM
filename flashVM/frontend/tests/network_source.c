/* Direct C source-policy contract and memory URL origin, independent of player argument parsing. */
#define SDL_MAIN_HANDLED
#include "player.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned int events, stopped;
static int notice(const FVM_DesktopLoadEvent *event, void *userdata)
{
    ++events;
    if (userdata && event->phase == FVM_DESKTOP_LOAD_BEGIN) { stopped = 1; return 1; }
    return 0;
}
int main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    FVM_DesktopSource source;
    FVM_DesktopHTTPPolicy policy;
    FVM_DesktopLoadMonitor monitor;
    unsigned char bytes[4096];
    FILE *file;
    const char *error = "sentinel";
    int status;
    if (argc != 4) return 2;
    if (!fvm_desktop_background_supported()) return 77;
    assert(!fvm_desktop_network_policy_init(0));
    assert(fvm_desktop_network_policy_init(&policy));
    fvm_desktop_load_monitor_init(&monitor);
    fvm_desktop_options_init(&options); options.frames = 700; options.fps = 1000;
    options.mute = 1; options.mode = FVM_EXECUTION_INTERPRETER; options.max_loads = 4;
    policy.ca_file = getenv("FLASHVM_CA_FILE");
    policy.max_redirects = 3; monitor.notify = notice;
    source.location = argv[1]; source.kind = FVM_DESKTOP_SOURCE_URL;
    source.bytes = 0; source.size = 0;
    if (!strcmp(argv[3], "memory")) {
        file = fopen(argv[2], "rb"); assert(file);
        source.size = fread(bytes, 1, sizeof(bytes), file); assert(source.size && source.size < sizeof(bytes));
        assert(!ferror(file)); fclose(file); source.bytes = bytes; source.kind = FVM_DESKTOP_SOURCE_MEMORY;
    } else if (!strcmp(argv[3], "invalid")) policy.max_redirects = FVM_HTTP_REDIRECT_MAX + 1;
    else if (!strcmp(argv[3], "stop")) monitor.userdata = &events;
    status = fvm_desktop_run_source_network(&options, &source, &policy, &monitor, &error);
    if (!strcmp(argv[3], "invalid")) assert(status == 2 && error && !events);
    else assert(status == 0 && !error && events && (!monitor.userdata || stopped));
    assert(!fvm_desktop_transfer_active() && fvm_desktop_shutdown() == FVM_OK);
    return 0;
}
