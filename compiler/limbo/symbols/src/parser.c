/* Reset recoverable state before each generated parse. */
#include "sblmapp.c"
/* Error distance and count belong to the generated parser instance. */
void sblmap_parser_reset(void)
{
    errors = 0;
    ErrDist = MinErrDist;
}
