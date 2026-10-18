/* Windows console entry: convert the real UTF-16 process command line to UTF-8. */
#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include "../common/player.h"
/* Avoid ANSI CRT argv corruption; SDL_RWFromFile accepts the resulting UTF-8 paths. */
int main(void)
{
    LPWSTR *wide;
    char **utf8;
    int argc = 0, i, status = 1;
    wide = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!wide)
        return 1;
    utf8 = (char **)calloc((size_t)argc, sizeof(*utf8));
    if (!utf8)
    {
        LocalFree(wide);
        return 1;
    }
    for (i = 0; i < argc; ++i)
    {
        int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide[i], -1, 0, 0, 0, 0);
        if (!bytes)
            goto cleanup;
        utf8[i] = (char *)malloc((size_t)bytes);
        if (!utf8[i] ||
            !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide[i], -1, utf8[i], bytes, 0, 0))
            goto cleanup;
    }
    status = fvm_desktop_main(argc, utf8);
cleanup:
    for (i = 0; i < argc; ++i)
        free(utf8[i]);
    free(utf8);
    LocalFree(wide);
    return status;
}
