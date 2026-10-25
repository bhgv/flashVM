/* Ordinary C89 application consumes only the separately linked module's public C header. */
#include "module.h" /* No VM or SDL header/dependency knowledge is needed in this source. */


/* Must be entered on the native UI thread. */
int main(void)                               /* No asset file or player CLI arguments. */
{                                            /* Begin the independent host application. */
    int status = embedded_movie_play();      /* The library loads its own embedded SWF. */
    int cleanup = embedded_movie_shutdown(); /* Finish that same library's SDK instance. */
    return status ? status : cleanup;        /* Preserve playback errors, otherwise report cleanup. */
} /* End the host session; no library may be unloaded while playback is active. */
