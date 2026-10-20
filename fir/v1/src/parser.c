/* Keep generated parser state private; reset before every library invocation. */
#include "fir1p.c"
void fir_parser_reset(void)
{
    errors = 0;
    ErrDist = MinErrDist;
}
