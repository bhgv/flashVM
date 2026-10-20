/* Generated parser state reset, invoked before the first token is read. */
int as3_lex_get(void);
#undef S_Get
#define S_Get as3_lex_get
#include "as3p.c"
void as3_parser_reset(void)
{
    allow_in = 1;
    errors = 0;
    ErrDist = MinErrDist;
}
