/* Coco/R still owns parsing; this wrapper resets its process-local state. */
#include "src1p.c"
void source_parser_reset(void)
{
    errors = 0;
    ErrDist = MinErrDist;
    depth = 0;
}
