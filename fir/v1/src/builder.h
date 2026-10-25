/* Semantic actions called only by the generated FIR parser. */
#ifndef FIR_BUILDER_H
#define FIR_BUILDER_H
#include "fir.h"


enum
{
    B_VERSION,
    B_FUNCTION,
    B_MODULE,
    B_RESULT,
    B_TYPE,
    B_BLOCK,
    B_A,
    B_B,
    B_YES,
    B_NO,
    B_LITERAL,
    B_LANGUAGE,
    B_SEMANTICS
};


void fir_build_token(int role, const char *text, int line);


void fir_build_begin(int line);


void fir_build_end(void);


void fir_build_op(FirOp op);


void fir_build_channels(void);


void fir_build_error(const char *message, int line);
#endif
