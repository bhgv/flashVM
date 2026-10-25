/* macOS desktop entry: SDL2's Cocoa backend runs on the process main thread. */
#include "../common/player.h"


/* Launch from a terminal with explicit SWF paths; this is not an app-bundle launcher. */
int main(int argc, char **argv)
{
    return fvm_desktop_main(argc, argv);
}
