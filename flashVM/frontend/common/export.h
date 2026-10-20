/* C89 symbol visibility for static archives and Windows player-host DLLs. */
#ifndef FVM_DESKTOP_EXPORT_H
#define FVM_DESKTOP_EXPORT_H
#if defined(_WIN32) && defined(FVM_DESKTOP_SHARED)
#ifdef FVM_DESKTOP_BUILD
#define FVM_DESKTOP_API __declspec(dllexport) /* Building the player-host DLL. */
#else
#define FVM_DESKTOP_API __declspec(dllimport) /* Consuming its import library. */
#endif
#else
#define FVM_DESKTOP_API /* Static archives and default ELF/Mach-O visibility. */
#endif
#endif
