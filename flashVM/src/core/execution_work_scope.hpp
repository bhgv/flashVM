/* FlashVM: shared execution-work accounting and worker cancellation for native traversal. */
#ifndef FVM_EXECUTION_WORK_SCOPE_HPP
#define FVM_EXECUTION_WORK_SCOPE_HPP /* Guard for the C++98 native traversal budget scope. */
#include "runtime.hpp"
#include "execution_work_constants.h"
#include "flashvm/worker.h"
#include <limits>


/* Share native traversal work with nested AVM1 callbacks instead of resetting their budget. */
class FVM_ExecutionWorkScope
{
    FVM_Context *context_; /* Context whose instruction/work counter is charged. */
    bool enabled_;         /* Whether this scope participates in native work accounting. */
    /* Keep execution-depth ownership unique. */
    FVM_ExecutionWorkScope(const FVM_ExecutionWorkScope &);
    /* Keep execution-depth ownership unique. */
    FVM_ExecutionWorkScope &operator=(const FVM_ExecutionWorkScope &);

  public:


    /* Enter the existing execution budget, resetting it only at an outer host call. */
    FVM_ExecutionWorkScope(FVM_Context *c, bool enabled = true) : context_(c), enabled_(enabled)
    {
        if (enabled_)
        {
            if (!c->avm1_execution_depth)
            {
                c->avm1_steps = 0;
            }
            ++c->avm1_execution_depth;
        }
    }


    /* Restore execution nesting after completion, callback errors and allocation exceptions. */
    ~FVM_ExecutionWorkScope()
    {
        if (enabled_)
        {
            --context_->avm1_execution_depth;
        }
    }


    /* Charge a unit and periodically honor worker cancellation without entering the collector. */
    FVM_Result step()
    {
        if (!enabled_)
        {
            return FVM_OK;
        }
        unsigned long limit = context_->runtime->config.instruction_limit;
        if (context_->avm1_steps == std::numeric_limits<unsigned long>::max() ||
            (limit && context_->avm1_steps >= limit))
        {
            return FVM_ERROR;
        }
        ++context_->avm1_steps;
        if ((context_->avm1_steps & FVM_EXECUTION_WORK_CANCEL_MASK) == FVM_EXECUTION_WORK_CANCEL_PHASE &&
            context_->worker)
        {
            int stopped = 0;
            if (fvm_worker_stop_requested(context_->worker, &stopped) == FVM_OK && stopped)
            {
                return FVM_ERROR;
            }
        }
        return FVM_OK;
    }
};
#endif
