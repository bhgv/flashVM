/* Shared desktop application entry; OS adapters supply process arguments only. */
#ifndef FVM_DESKTOP_PLAYER_H
#define FVM_DESKTOP_PLAYER_H
#include "flashvm/flashvm.h"
/* Borrowed build metadata; no VM, device, window or process locale is initialized. */
typedef struct FVM_DesktopBuildInfo
{
    const char *version;       /* Static development version from VERSION.txt at configure time. */
    unsigned long abi_version; /* Linked VM ABI, independent of the development version. */
    unsigned int pointer_bits; /* Width of this target's data pointer, not a host CPU detector. */
    int jit_available;      /* Linked backend availability, not per-body coverage/device proof. */
    int sdl_audio_compiled; /* Queue bridge compiled in; no audio endpoint is probed. */
} FVM_DesktopBuildInfo;
#ifdef __cplusplus
extern "C"
{
#endif
    /* Fill caller storage without allocating; reject a null output pointer. */
    FVM_Result fvm_desktop_build_info(FVM_DesktopBuildInfo *out);
    /* Return 0 on help/version/normal close, 1 on playback failure, 2 on invalid command line.
     * argc must be positive; argv must contain argc non-null pointers to NUL-terminated strings.
     * Invalid pointer entries/counts fail before SDL/file/VM initialization. The caller
     * owns accessible storage throughout the synchronous call; no argv[argc] is read.
     * Playback must run on the native UI/VM owner thread; concurrent/reentrant calls
     * are not supported. The metadata query above does not enter the VM or collector. */
    int fvm_desktop_main(int argc, char **argv);
#ifdef __cplusplus
}
#endif
#endif
