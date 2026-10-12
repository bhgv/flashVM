/* FlashVM: Public embedding API declarations: stack.
 * File: include/flashvm/stack.hpp.
 */

#ifndef FLASHVM_STACK_HPP
/* Define FLASHVM STACK HPP constant or compilation switch. */
#define FLASHVM_STACK_HPP
#include "stack.h"
/* C++98 convenience wrapper; the C API remains the stable ABI. */
namespace flashvm
{
/* Define Stack class and its representation. */
class Stack
{
    FVM_Stack *stack_; /* Execution stack or stack API handle. */
    /* Initialize Stack and its owned state. */
    Stack(const Stack &);
    Stack &operator=(const Stack &); /* Copy assignment is unavailable for this owning wrapper. */

  public:
    /* Initialize Stack and its owned state. */
    explicit Stack(FVM_Context *context) : stack_(0)
    {
        fvm_stack_create(context, &stack_);
    }
    /* Release the resources owned by this Stack instance. */
    ~Stack()
    {
        fvm_stack_destroy(stack_);
    }
    /* Compute and return valid. */
    bool valid() const
    {
        return stack_ != 0;
    }
    /* Return the number of stored elements. */
    unsigned int size() const
    {
        return fvm_stack_size(stack_);
    }
    /* Push the current value. */
    FVM_Result push(const FVM_Value &value)
    {
        return fvm_stack_push(stack_, &value);
    }
    /* Read the current value. */
    FVM_Result get(int index, FVM_Value &value) const
    {
        return fvm_stack_get(stack_, index, &value);
    }
    /* Pop the current value. */
    FVM_Result pop(unsigned int n = 1)
    {
        return fvm_stack_pop(stack_, n);
    }
    /* Compute and return native handle. */
    FVM_Stack *native_handle()
    {
        return stack_;
    }
};
} // namespace flashvm
#endif
