/* FlashVM: shared execution-work accounting and worker cancellation for native Array traversal. */
#ifndef FVM_ARRAY_WORK_HPP
#define FVM_ARRAY_WORK_HPP /* Guard for the C++98 native traversal budget scope. */
#include "../core/runtime.hpp"
#include "flashvm/worker.h"
#include <limits>
/* Share native traversal work with nested AVM1 callbacks instead of resetting their budget. */
class FVM_ArrayWorkScope
{
    FVM_Context *context_; /* Context whose instruction/work counter is charged. */
    bool enabled_; /* Whether this scope participates in native Array accounting. */
    /* Keep execution-depth ownership unique. */
    FVM_ArrayWorkScope(const FVM_ArrayWorkScope &);
    /* Keep execution-depth ownership unique. */
    FVM_ArrayWorkScope &operator=(const FVM_ArrayWorkScope &);
  public:
    /* Enter the existing execution budget, resetting it only at an outer host call. */
    FVM_ArrayWorkScope(FVM_Context *c, bool enabled = true) : context_(c), enabled_(enabled)
    {
        if (enabled_)
        {
            if (!c->avm1_execution_depth)
                c->avm1_steps = 0;
            ++c->avm1_execution_depth;
        }
    }
    /* Restore execution nesting after completion, callback errors and allocation exceptions. */
    ~FVM_ArrayWorkScope()
    {
        if (enabled_)
            --context_->avm1_execution_depth;
    }
    /* Charge a unit and periodically honor worker cancellation without entering the collector. */
    FVM_Result step()
    {
        if (!enabled_)
            return FVM_OK;
        unsigned long limit = context_->runtime->config.instruction_limit;
        if (context_->avm1_steps == std::numeric_limits<unsigned long>::max() ||
            (limit && context_->avm1_steps >= limit))
            return FVM_ERROR;
        ++context_->avm1_steps;
        if ((context_->avm1_steps & 63UL) == 1UL && context_->worker)
        {
            int stopped = 0;
            if (fvm_worker_stop_requested(context_->worker, &stopped) == FVM_OK && stopped)
                return FVM_ERROR;
        }
        return FVM_OK;
    }
};
#endif
