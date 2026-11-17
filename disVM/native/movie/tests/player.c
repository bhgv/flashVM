/* Copyright (c) 2026 bhgv
 * EN: Actual independent FlashVM player loads nested SWF/data through a TCP Styx service.
 * RU: Настоящий независимый FlashVM player загружает nested SWF/data через TCP Styx service.
 */
#define SDL_MAIN_HANDLED
#include "../client.h"
#include "player.h"
#include <SDL.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Connect only to the fixture's granted local port; caller transfers fd to native client.
 * Подключиться к granted local port теста; caller передаёт fd native client. */
static int connectserver(const char *port)
{
    struct sockaddr_in address;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)atoi(port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (fd < 0 || connect(fd, (struct sockaddr *)&address, sizeof(address)))
    {
        if (fd >= 0)
        {
            close(fd);
        }
        return -1;
    }
    return fd;
}


/* EN/RU: Deliberately refuse one borrowed byte chunk; no state or buffer retained.
 * Явно отклонить один borrowed chunk; state/buffer не удерживается. */
static FVM_Result refuse(const void *bytes, size_t size, void *state)
{
    (void)bytes;
    if (!size)
    {
        return FVM_ERROR;
    }
    ++*(unsigned *)state;
    return FVM_UNSUPPORTED;
}


/* EN/RU: Run one real movie or verify explicit denial; close all transport/player ownership.
 * Small negotiated msize forces fragmented request/response records on real TCP.
 * Выполнить real movie либо проверить отказ; малый msize фрагментирует S-records через TCP. */
int main(int argc, char **argv)
{
    if (argc != 5)
    {
        return 2;
    }
    int fd = connectserver(argv[1]);
    if (fd < 0)
    {
        return 3;
    }
    StyxClient *client = styxclientmountfd(fd, 256, "fixture", "", 3000, 8);
    if (!client)
    {
        return 4;
    }
    if (!strcmp(argv[4], "sink"))
    {
        char origin[4096];
        unsigned calls = 0;
        FVM_Result result = styxmoviefetch(FVM_DESKTOP_FETCH_CODE, argv[3], NULL,
            refuse, &calls, origin, sizeof(origin), client);
        if (result != FVM_UNSUPPORTED || calls != 1)
        {
            styxclientshutdown(client);
            styxclientfree(client);
            return 6;
        }
    }
    FVM_DesktopOptions options;
    FVM_DesktopManagedLoader loader;
    FVM_DesktopSource source;
    const char *error = NULL;
    fvm_desktop_options_init(&options);
    options.mode = !strcmp(argv[2], "jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    options.mute = 1;
    options.frames = 4;
    options.stats = 1;
    fvm_desktop_managed_loader_init(&loader);
    loader.fetch = styxmoviefetch;
    loader.userdata = client;
    loader.max_loads = 6;
    memset(&source, 0, sizeof(source));
    source.kind = FVM_DESKTOP_SOURCE_FILE;
    source.location = argv[3];
    int result = fvm_desktop_run_source_managed(&options, &source, &loader, &error);
    int expected = !strcmp(argv[4], "deny") ? FVM_DESKTOP_EXIT_PLAYBACK : FVM_DESKTOP_EXIT_OK;
    styxclientshutdown(client);
    styxclientfree(client);
    SDL_Quit();
    if (result != expected || (!result && error))
    {
        fprintf(stderr, "movie player: %d expected %d: %s\n", result, expected, error ? error : "none");
        return 5;
    }
    puts("PASS real TCP Styx S-record managed movie");
    return 0;
}
