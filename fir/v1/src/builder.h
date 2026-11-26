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
    B_SEMANTICS,
    B_AVM1_METHOD, /* Exact root method symbol. */
    B_AVM1_PARAMETER /* Next formal name of the root method. */
};


void fir_build_token(int role, const char *text, int line);


void fir_build_begin(int line);


void fir_build_end(void);


void fir_build_op(FirOp op);


void fir_build_channels(void);

/* EN/RU: Mark the frame entry / Пометить обработчик кадра. */
void fir_build_avm1_frame(void);

void fir_build_error(const char *message, int line);
#endif
