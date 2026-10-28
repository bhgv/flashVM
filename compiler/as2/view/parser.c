/* Generated parser wrapper; reset error-distance state for sequential library calls. */
#include "viewp.c"

/* Clear only generated parser state; the scanner resets its owned buffer in Parse(). */
void view_parser_reset(void)
{
    errors = 0;
    ErrDist = MinErrDist;
}
