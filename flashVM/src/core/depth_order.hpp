/* FlashVM: budgeted ascending depth sorting shared by timeline, raster and retained input traversal. */
#ifndef FVM_CORE_DEPTH_ORDER_HPP
#define FVM_CORE_DEPTH_ORDER_HPP /* Private C++98 depth-sort guard. */
#include "execution_work_scope.hpp"

namespace fvm_depth_order
{ /* Restore a max-heap, charging each comparison block so sorting shares the caller's budget. */


inline FVM_Result
sift(fvm_detail::vector<unsigned long> &order, size_t root, size_t count, FVM_ExecutionWorkScope &work)
{
    while (root < count / 2)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
        {
            return result;
        }
        size_t child = root * 2 + 1;
        if (child + 1 < count && order[child] < order[child + 1])
        {
            ++child;
        }
        if (order[root] >= order[child])
        {
            break;
        }
        unsigned long value = order[root];
        order[root] = order[child];
        order[child] = value;
        root = child;
    }
    return FVM_OK;
}


/* Sort native depth keys without STL or quadratic insertion sorting. */
inline FVM_Result sort(fvm_detail::vector<unsigned long> &order, FVM_ExecutionWorkScope &work)
{
    for (size_t i = order.size() / 2; i; --i)
    {
        FVM_Result result = sift(order, i - 1, order.size(), work);
        if (result != FVM_OK)
        {
            return result;
        }
    }
    for (size_t count = order.size(); count > 1; --count)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
        {
            return result;
        }
        unsigned long value = order[0];
        order[0] = order[count - 1];
        order[count - 1] = value;
        result = sift(order, 0, count - 1, work);
        if (result != FVM_OK)
        {
            return result;
        }
    }
    return FVM_OK;
}
} // namespace fvm_depth_order
#endif
