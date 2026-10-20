/* C++98 links only the player-host boundary, including DLL imports and opaque cleanup. */
#include "../common/player.h"
#ifdef FVM_DESKTOP_SOCKET_HTTP
#include "../common/socket_http.h"
#endif
/* No SDL or private VM header, no process argument vector and no native device startup. */
int main()
{
#ifdef FVM_DESKTOP_SOCKET_HTTP
    FVM_SocketHTTPOptions network_options;
    if (!fvm_desktop_socket_options_init(&network_options) || network_options.ca_file ||
        fvm_desktop_socket_capabilities() != fvm_desktop_network_capabilities())
        return 1;
#endif
    FVM_DesktopOptions options;
    FVM_DesktopLoader loader;
    FVM_DesktopBuildInfo info;
    const char *error = 0;
    unsigned int network = fvm_desktop_network_capabilities();
    if (network & ~FVM_DESKTOP_NETWORK_MASK)
        return 1;
    if ((network & FVM_DESKTOP_NETWORK_SOCKET) && (network & FVM_DESKTOP_NETWORK_CURL))
        return 1;
    if ((network & FVM_DESKTOP_NETWORK_HTTPS) && !(network & FVM_DESKTOP_NETWORK_HTTP))
        return 1;
    if (!fvm_desktop_options_init(&options) || !fvm_desktop_loader_init(&loader))
        return 1;
    if (fvm_desktop_build_info(&info) != FVM_OK || !info.version)
        return 1;
    if (fvm_desktop_run_memory(&options, 0, 0, "invalid", &error) != FVM_DESKTOP_EXIT_ARGUMENT ||
        !error)
        return 1;
    if (fvm_desktop_run_source(&options, 0, &loader, 0) != FVM_DESKTOP_EXIT_ARGUMENT)
        return 1;
    return fvm_desktop_shutdown() == FVM_OK ? 0 : 1;
}
