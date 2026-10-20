/* Ordinary C89 application consuming the separately linked resource module. */
#define SDL_MAIN_HANDLED /* Keep this native entrypoint. */
#include <SDL.h> /* The application owns the complete SDL session. */
#include "module.h" /* Only the module's public C symbol is needed. */

/* Must be entered on the native UI thread. */
int main(void) /* No asset file or player CLI arguments. */
{ /* Begin the independent host application. */
    int status = embedded_movie_play(); /* Library loads its own embedded SWF. */
    SDL_Quit(); /* All module/player-owned views have already been released. */
    return status; /* Propagate the documented player status. */
} /* End the host session. */
