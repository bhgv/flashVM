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
    FVM_DesktopTransferOptions transfer;
    if (!fvm_desktop_transfer_options_init(&transfer) || transfer.fetch ||
        fvm_desktop_transfer_active() || fvm_desktop_transfer_release(0) != FVM_OK)
        return 1;
    if (fvm_desktop_transfer_stop_requested(0))
        return 1;
    if (fvm_desktop_transfer_begin_timed(0, 0, &transfer, 1, 0) != FVM_INVALID_ARGUMENT)
        return 1;
    if (fvm_desktop_background_supported() != 0 && fvm_desktop_background_supported() != 1)
        return 1;
    FVM_DesktopBackgroundLoader background;
    if (!fvm_desktop_background_loader_init(&background) || background.transfer.fetch ||
        background.max_loads != 1 ||
        fvm_desktop_run_source_background(0, 0, &background, 0) != FVM_DESKTOP_EXIT_ARGUMENT)
        return 1;
    if (fvm_desktop_run_source_async(0, 0, &background, 0) != FVM_DESKTOP_EXIT_ARGUMENT ||
        fvm_desktop_run_async(0, 0) != FVM_DESKTOP_EXIT_ARGUMENT)
        return 1;
    FVM_DesktopLoadMonitor monitor;
    if (!fvm_desktop_load_monitor_init(&monitor) || monitor.asynchronous != 1 ||
        monitor.timeout_ms || monitor.notify || monitor.userdata ||
        fvm_desktop_run_source_monitored(0, 0, &background, &monitor, 0) !=
            FVM_DESKTOP_EXIT_ARGUMENT)
        return 1;
    if (fvm_desktop_run_monitored(0, &monitor, 0) != FVM_DESKTOP_EXIT_ARGUMENT)
        return 1;
    FVM_DesktopHTTPPolicy policy;
    if (!fvm_desktop_network_policy_init(&policy) || policy.max_redirects || !policy.connect_ms ||
        policy.connect_ms > policy.total_ms || policy.ca_file ||
        fvm_desktop_run_source_network(0, 0, &policy, &monitor, 0) != FVM_DESKTOP_EXIT_ARGUMENT ||
        fvm_desktop_run_network(0, &policy, &monitor, 0) != FVM_DESKTOP_EXIT_ARGUMENT)
        return 1;
    FVM_DesktopPlaybackControl control;
    if (!fvm_desktop_playback_control_init(&control) || control.struct_size != sizeof(control) ||
        control.initially_paused || control.notify || control.userdata ||
        fvm_desktop_run_source_controlled(0, 0, 0, &control, 0) != FVM_DESKTOP_EXIT_ARGUMENT ||
        fvm_desktop_run_source_background_controlled(0, 0, &background, &monitor, &control, 0) !=
            FVM_DESKTOP_EXIT_ARGUMENT)
        return 1;
    control.initially_paused = 1;
    if (fvm_desktop_run_source_controlled(0, 0, 0, &control, 0) != FVM_DESKTOP_EXIT_ARGUMENT)
        return 1;
    FVM_DesktopPlaybackMailbox *mailbox = 0;
    FVM_DesktopPlaybackSnapshot snapshot;
    if (!fvm_desktop_playback_snapshot_init(&snapshot) ||
        fvm_desktop_playback_mailbox_create(&mailbox) != FVM_OK ||
        fvm_desktop_playback_mailbox_poll(mailbox, &snapshot) != FVM_NOT_FOUND ||
        fvm_desktop_playback_mailbox_active() != 1 ||
        fvm_desktop_playback_mailbox_notify(0, mailbox) != FVM_DESKTOP_PLAYBACK_STOP ||
        !fvm_desktop_playback_mailbox_load_notify(0, mailbox) ||
        fvm_desktop_playback_mailbox_post(mailbox, FVM_DESKTOP_PLAYBACK_STOP) != FVM_OK ||
        !fvm_desktop_playback_mailbox_stop_requested(mailbox) ||
        fvm_desktop_shutdown() != FVM_ERROR ||
        fvm_desktop_playback_mailbox_release(mailbox) != FVM_OK)
        return 1;
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
