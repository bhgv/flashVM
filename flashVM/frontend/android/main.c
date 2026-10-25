/* Android SDLActivity entry: SDL owns the Surface/native thread and passes selected test arguments.
 */
#define SDL_MAIN_HANDLED
#include "../common/player.h"
#include <SDL.h>


/* Export the symbol SDLActivity resolves with dlsym after loading libflashvm_android.so. */
DECLSPEC int SDL_main(int argc, char **argv)
{
    return fvm_desktop_main(argc, argv);
}
