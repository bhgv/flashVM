/* FlashVM: native structured exception handling over shared rooted execution state. */
#include "internal.hpp"
#include "try_constants.h"
#include "../avm1/try_record.hpp"
#include "../avm1/try_constants.h"
#include "../avm1/lexical_scope.hpp"
#include "flashvm/exception.h"
/* [GC:] Baseline operands, caught/pending exceptions and prior Return values survive each block. */
FVM_Result fvm_jit_try(FVM_JitInvocation *s, unsigned int index)
{
    if (index >= s->code->constants.size())
        return FVM_INVALID_ARGUMENT;
    const FVM_JitConstant &descriptor = s->code->constants[index];
    if (descriptor.text.size() < FVM_JIT_TRY_PAYLOAD_OFFSET ||
        descriptor.value.type != FVM_VALUE_NUMBER || descriptor.value.number < 0 ||
        descriptor.value.number > s->code->blocks.size())
        return FVM_INVALID_ARGUMENT;
    unsigned int first = (unsigned int)descriptor.value.number;
    if (s->code->blocks.size() - first < FVM_AVM1_TRY_BLOCK_COUNT)
        return FVM_INVALID_ARGUMENT;
    const unsigned char *data = (const unsigned char *)descriptor.text.data();
    size_t length = data[FVM_JIT_TRY_LENGTH_OFFSET] |
        ((size_t)data[FVM_JIT_TRY_LENGTH_OFFSET + 1] << FVM_JIT_BYTE_BITS);
    FVM_TryRecord record;
    if (!fvm_try_record(data, descriptor.text.size(), FVM_JIT_TRY_PAYLOAD_OFFSET, length, record))
        return FVM_INVALID_ARGUMENT;
    FVM_Context *c = s->context;
    FVM_ExecutionRoots baseline(c, 0);
    baseline.values = *s->stack;
    FVM_Result result = fvm_jit_run_block(s, first + FVM_AVM1_TRY_BODY_INDEX);
    FVM_ExecutionRoots saved(c, FVM_AVM1_TRY_ROOT_COUNT);
    if (result == FVM_EXCEPTION && (record.flags & FVM_AVM1_TRY_CATCH))
    {
        saved.values[FVM_AVM1_TRY_THROWN_ROOT] = c->exception_value;
        *s->stack = baseline.values;
        fvm_exception_clear(c);
        /* [GC:] Host root catches own a temporary activation, including register catches. */
        FVM_AVM1LexicalScope catch_scope(c);
        FVM_ScriptFrame catching = *s->frame;
        FVM_Object *activation = catching.activation;
        if (!c->swf_loaded && !activation)
        {
            FVM_Result created = fvm_object_create(c, &activation);
            if (created != FVM_OK)
                return created;
            activation->prototype = c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back();
            catch_scope.publish(activation);
            catching.activation = activation;
        }
        bool had_binding = false;
        if (record.flags & FVM_AVM1_TRY_CATCH_REGISTER)
        {
            if (s->frame->registers == c->avm1_global_registers)
            {
                FVM_Value ignored;
                FVM_Result stored = fvm_exec_global_register(c, 1, record.reg,
                    &saved.values[FVM_AVM1_TRY_THROWN_ROOT], &ignored);
                if (stored != FVM_OK)
                    return stored;
            }
            else if (record.reg < s->frame->register_count)
            {
                if (!s->frame->registers)
                    return FVM_INVALID_ARGUMENT;
                s->frame->registers[record.reg] = saved.values[FVM_AVM1_TRY_THROWN_ROOT];
            }
            else if (!c->swf_loaded)
                return FVM_INVALID_ARGUMENT;
        }
        else if (c->swf_loaded)
        {
            FVM_Result bound = fvm_jit_bind_catch(s, record.name,
                                                 &saved.values[FVM_AVM1_TRY_THROWN_ROOT]);
            if (bound != FVM_OK)
                return bound;
        }
        else
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator old =
                activation->properties.find(record.name);
            had_binding = old != activation->properties.end();
            if (had_binding)
                saved.values[FVM_AVM1_TRY_BINDING_ROOT] = old->second;
            activation->properties[record.name] = saved.values[FVM_AVM1_TRY_THROWN_ROOT];
        }
        result = fvm_jit_run_block(s, first + FVM_AVM1_TRY_CATCH_INDEX, &catching);
        if (!c->swf_loaded && !(record.flags & FVM_AVM1_TRY_CATCH_REGISTER))
        {
            if (had_binding)
                activation->properties[record.name] = saved.values[FVM_AVM1_TRY_BINDING_ROOT];
            else
                activation->properties.erase(record.name);
        }
    }
    if ((record.flags & FVM_AVM1_TRY_FINALLY) && (result == FVM_OK || result == FVM_EXCEPTION))
    {
        bool was_return = s->returned;
        bool was_exception = c->exception_pending;
        bool was_fault = c->fault_pending;
        FVM_ExceptionInfo fault = c->fault;
        saved.values[FVM_AVM1_TRY_RETURN_ROOT] = s->result;
        saved.values[FVM_AVM1_TRY_PENDING_ROOT] = c->exception_value;
        s->returned = false;
        fvm_exception_clear(c);
        FVM_Result final_result = fvm_jit_run_block(s, first + FVM_AVM1_TRY_FINALLY_INDEX);
        if (final_result != FVM_OK || s->returned)
            result = final_result;
        else
        {
            s->returned = was_return;
            s->result = saved.values[FVM_AVM1_TRY_RETURN_ROOT];
            c->exception_pending = was_exception;
            c->exception_value = saved.values[FVM_AVM1_TRY_PENDING_ROOT];
            c->fault_pending = was_fault;
            c->fault = fault;
        }
    }
    s->condition = s->returned;
    return result;
}
