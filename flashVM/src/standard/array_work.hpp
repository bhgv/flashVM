/* FlashVM: preserve the Array traversal interface over shared native execution accounting. */
#ifndef FVM_ARRAY_WORK_HPP
#define FVM_ARRAY_WORK_HPP /* Native Array work interface guard. */
#include "../core/execution_work_scope.hpp"

/* Existing Array callers retain identical budget nesting and cancellation behavior. */
typedef FVM_ExecutionWorkScope FVM_ArrayWorkScope;
#endif
