/* Generated parser state reset, invoked before the first token is read. */
#include "as2p.c"
void as2_parser_reset(void)
{
    allow_in = 1;
    errors = 0;
    ErrDist = MinErrDist;
}
