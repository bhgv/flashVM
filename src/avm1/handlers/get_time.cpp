/* FlashVM: context-relative elapsed milliseconds shared by interpreter and generated code. */
#include "flashvm/execution.h"
#include "../../core/runtime.hpp"
#include "../../core/timer.hpp"
#include "../../core/numeric_constants.h"
#include <cmath>
/* Read elapsed milliseconds, reject failed samples and wrap the integer result modulo 2^32. */
extern "C" FVM_Result fvm_exec_get_time(FVM_Context *context, FVM_Value *output)
{
    if (!context || context->destroying || context->gc_releasing || !output)
        return FVM_INVALID_ARGUMENT;
    double sample = 0;
    if (!fvm_timer_read(&sample))
        return FVM_ERROR;
    if (!context->timer_clock_valid)
    {
        /* Recover a context whose initial platform clock read was unavailable. */
        context->timer_origin_ms = sample;
        context->timer_clock_valid = true;
    }
    double elapsed = sample >= context->timer_origin_ms ? sample - context->timer_origin_ms : 0;
    if (elapsed < context->timer_elapsed_ms)
        elapsed = context->timer_elapsed_ms;
    context->timer_elapsed_ms = elapsed;
    fvm_value_number(output, std::fmod(std::floor(elapsed), FVM_UINT32_MODULUS));
    return FVM_OK;
}
