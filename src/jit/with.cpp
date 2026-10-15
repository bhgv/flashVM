/* FlashVM: compiled lexical With blocks over shared rooted execution state. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/lexical_scope.hpp"
/* [GC:] Keep the wrapped input rooted until its managed lexical node is published. */
FVM_Result fvm_jit_with(FVM_JitInvocation *s, unsigned int index)
{
    if (index >= s->code->blocks.size())
        return FVM_INVALID_ARGUMENT;
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_value(c->swf_loaded, *s->stack);
    if (status != FVM_OK)
        return status;
    s->condition = false;
    const FVM_Value &value = s->stack->back();
    if (value.type == FVM_VALUE_NULL || value.type == FVM_VALUE_UNDEFINED)
    {
        s->stack->pop_back();
        return FVM_OK;
    }
    FVM_Object *scope = 0;
    status = fvm_exec_with_scope(c, c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back(),
                                 &value, &scope);
    if (status != FVM_OK)
        return status;
    FVM_AVM1LexicalScope lexical(c);
    lexical.publish(scope);
    s->stack->pop_back();
    FVM_JitBlock &block = s->code->blocks[index];
    FVM_JitInvocation child = *s;
    child.code = block.code;
    child.source_base += (unsigned long)block.offset;
    child.returned = false;
    fvm_value_undefined(&child.result);
    /* Protect the child's executable mapping during collecting/reentrant callbacks. */
    struct Active
    {
        FVM_JitCode *code; /* Child mapping owned by the suspended parent. */
        /* Publish child activity before entering generated instructions. */
        explicit Active(FVM_JitCode *c) : code(c) { ++code->active; }
        /* Restore activity on every helper status and C++ unwind path. */
        ~Active() { --code->active; }
    } active(block.code);
    typedef FVM_Result (*Entry)(FVM_JitInvocation *);
    Entry entry;
    std::memcpy(&entry, &block.code->memory, sizeof(entry));
    status = entry(&child);
    /* Match the interpreter's shared Execution, including failed/returning blocks. */
    s->pool = child.pool;
    s->steps = child.steps;
    s->condition = child.returned;
    if (child.returned)
    {
        s->returned = true;
        s->result = child.result;
    }
    return status;
}
