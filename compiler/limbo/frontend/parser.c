/* Generated parser state reset, invoked before the first token is read. */
#include "limbop.c"


void limbo_parser_reset(void)
{
    errors = 0;
    ErrDist = MinErrDist;
}
