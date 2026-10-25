/* Generated parser state reset, invoked before the first token is read. */
#include "firsynp.c"


void firsyn_parser_reset(void)
{
    errors = 0;
    ErrDist = MinErrDist;
}
