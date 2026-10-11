#ifndef FLASHVM_STACK_HPP
#define FLASHVM_STACK_HPP
#include "stack.h"
/* C++98 convenience wrapper; the C API remains the stable ABI. */
namespace flashvm {
class Stack {
    FVM_Stack *stack_;
    Stack(const Stack &);
    Stack &operator=(const Stack &);
public:
    explicit Stack(FVM_Context *context):stack_(0) { fvm_stack_create(context,&stack_); }
    ~Stack() { fvm_stack_destroy(stack_); }
    bool valid() const { return stack_!=0; }
    unsigned int size() const { return fvm_stack_size(stack_); }
    FVM_Result push(const FVM_Value &value) { return fvm_stack_push(stack_,&value); }
    FVM_Result get(int index,FVM_Value &value) const { return fvm_stack_get(stack_,index,&value); }
    FVM_Result pop(unsigned int n=1) { return fvm_stack_pop(stack_,n); }
    FVM_Stack *native_handle() { return stack_; }
};
}
#endif
