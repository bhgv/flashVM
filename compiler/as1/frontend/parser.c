/* Generated parser state reset, invoked before the first token is read. */
int as1_lex_get(void);
#undef S_Get
#define S_Get as1_lex_get
#include "as1p.c"


void as1_parser_reset(void)
{
    allow_in = 1;
    errors = 0;
    ErrDist = MinErrDist;
}
