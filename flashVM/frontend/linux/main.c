/* Linux desktop entry: SDL2 selects X11/Wayland through the user's session. */
#include "../common/player.h"
/* SDL/video and VM stay on this process's main thread. */
int main(int argc, char **argv)
{
    return fvm_desktop_main(argc, argv);
}
