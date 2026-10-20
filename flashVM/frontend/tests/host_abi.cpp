/* C++98 links only the player-host boundary, including DLL imports and opaque cleanup. */
#include "../common/player.h"
/* No SDL or private VM header, no process argument vector and no native device startup. */
int main()
{
    FVM_DesktopOptions options;
    FVM_DesktopLoader loader;
    FVM_DesktopBuildInfo info;
    const char *error = 0;
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
