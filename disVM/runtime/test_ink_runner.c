/* Real Dis runner with native frozen-ink fixtures; production main is unchanged. */
#define main ink_runtime_main
#include "main.c"
#undef main
#include "editor_ink.h"
#ifdef DVM_EDITOR_SDL
int dvm_test_live_ink(const char *font); /* Native SDL regression entry. */
#endif


/* Prepare two disjoint captured trajectories before the real runtime starts.
 * Only tests seed the registry; production fills it from native SDL capture. */
int main(int argc, char **argv)
{
    unsigned long token;
    unsigned int stroke;
#ifdef DVM_EDITOR_SDL
    if (argc == 3 && !strcmp(argv[1], "--sdl-fixture"))
    {
        return dvm_test_live_ink(argv[2]);
    }
#endif
    for (stroke = 0; stroke < 2; ++stroke)
    {
        unsigned int index;
        long x = stroke ? 500 : 100;
        long y = stroke ? 400 : 100;
        token = dvm_ink_begin(1, 0x245ac4, 8, y * 1001 + x);
        if (token != stroke + 1 || dvm_ink_read(token)[0])
        {
            fputs("FAIL ink fixture token/live copy\n", stderr);
            return 1;
        }
        for (index = 0; index < 48; ++index)
        {
            if (!dvm_ink_point(token, (y + index / 2) * 1001 + x + index * 2))
            {
                fputs("FAIL ink fixture capacity\n", stderr);
                return 1;
            }
        }
        if (!dvm_ink_end(token, (y + 24) * 1001 + x + 96))
        {
            return 1;
        }
    }
    return ink_runtime_main(argc, argv);
}
