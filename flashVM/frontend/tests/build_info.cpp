/* C++98 host links the C89 player boundary without C++ name mangling or SDL startup. */
#include "../common/player.h"
#include <cstring>

/* Both metadata and CLI adapters must work with nonexistent native device drivers. */
int main()
{
    FVM_DesktopBuildInfo info;
    char program[] = "embedded-player";
    char version[] = "--version";
    char *arguments[] = {program, version};
    if (fvm_desktop_build_info(0) != FVM_INVALID_ARGUMENT ||
        fvm_desktop_build_info(&info) != FVM_OK || !info.version ||
        !std::strlen(info.version) || info.abi_version != fvm_abi_version())
        return 1;
    return fvm_desktop_main(2, arguments);
}
