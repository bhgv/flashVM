/* FlashVM: context-owned execution policy and bounded process-local code cache. */
#ifndef FVM_EXECUTION_MODE_STATE_HPP
#define FVM_EXECUTION_MODE_STATE_HPP /* Execution state guard. */
#include "flashvm/execution_mode.h"
#include "flashvm/debug.h"
#include "../details/vector.hpp"
#include "../details/hash_table.hpp"
#include "../details/string.hpp"
#include <cstring>
/* No managed references: graph transfer retains the destination context's own policy. */
struct FVM_ExecutionModeState
{
    fvm_detail::map<fvm_detail::string, fvm_detail::vector<unsigned long> >
        breakpoints; /* Owned body names and exact offsets; no GC edges. */
    FVM_DebugBreakpointHandler breakpoint_handler; /* Borrowed synchronous host debugger. */
    void *breakpoint_data;  /* Borrowed debugger state on the owning VM thread. */
    bool breakpoint_busy;   /* Prevent recursive callback dispatch. */
    FVM_ExecutionMode mode; /* Requested mode; diagnostics can temporarily force interpretation. */
    unsigned int flags;     /* Opt-in replay policy. */
    unsigned int replay_depth;  /* Suppresses recursive retries and native reentry during replay. */
    FVM_ExecutionLogger logger; /* Borrowed host callback, or null for stderr. */
    void *logger_data;          /* Borrowed host callback state. */
    FVM_ExecutionStats stats;   /* Lightweight counters; no per-instruction native logging. */
    fvm_detail::map<fvm_detail::string, FVM_JitCode *>
        cache;          /* Owned source keys and code/null negatives. */
    size_t cache_bytes; /* Total emitted bytes owned by the automatic cache. */
    const fvm_detail::vector<fvm_detail::string> *native_pool;
        /* Borrowed prepared constant-pool snapshot during a native body; no GC edges. */
    /* Preserve the previous manual native-body ABI unless options are explicitly selected. */
    FVM_ExecutionModeState()
        : breakpoint_handler(0), breakpoint_data(0), breakpoint_busy(false),
          mode(FVM_EXECUTION_MANUAL), flags(0), replay_depth(0), logger(0), logger_data(0),
          cache_bytes(0), native_pool(0)
    {
        std::memset(&stats, 0, sizeof(stats));
        stats.struct_size = sizeof(stats);
    }
    /* Context shutdown occurs only after every active call has returned. */
    ~FVM_ExecutionModeState();

  private:
    /* A context code cache has unique mapping ownership. */
    FVM_ExecutionModeState(const FVM_ExecutionModeState &);
    /* Copying executable ownership would permit double release. */
    FVM_ExecutionModeState &operator=(const FVM_ExecutionModeState &);
};
#endif
