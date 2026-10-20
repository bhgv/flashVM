/* Generated parser state reset, invoked before the first token is read. */
int as2_lex_get(void);
#undef S_Get
#define S_Get as2_lex_get
#include "as2p.c"
void as2_parser_reset(void)
{
    allow_in = 1;
    errors = 0;
    ErrDist = MinErrDist;
}
