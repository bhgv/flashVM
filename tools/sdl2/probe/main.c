/* Verify a complete SDL2 header/library pair without opening a display or audio device. */
#define SDL_MAIN_HANDLED /* This diagnostic supplies its ordinary native main entry. */
#include <SDL.h>
#include <stdio.h>


/* Print compiled driver availability; no parameters. Return nonzero on an obsolete SDK.
 * A listed driver is not a successful native window/device acceptance test. */
int main(void)
{
    SDL_version version;
    int index;
    SDL_GetVersion(&version);
    if (SDL_VERSIONNUM(version.major, version.minor, version.patch) < SDL_VERSIONNUM(2, 0, 9))
    {
        fprintf(stderr, "SDL2 runtime is older than the required 2.0.9\n");
        return 1;
    }
    printf(
        "SDL2 %u.%u.%u\nvideo:", (unsigned int)version.major, (unsigned int)version.minor, (unsigned int)version.patch);
    for (index = 0; index < SDL_GetNumVideoDrivers(); index++)
    {
        printf(" %s", SDL_GetVideoDriver(index));
    }
    printf("\naudio:");
    for (index = 0; index < SDL_GetNumAudioDrivers(); index++)
    {
        printf(" %s", SDL_GetAudioDriver(index));
    }
    printf("\nDriver listing is build capability, not native window/audio acceptance.\n");
    return 0;
}
