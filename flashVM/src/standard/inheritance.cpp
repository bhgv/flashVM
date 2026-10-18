/* FlashVM: AVM1 extends/implements operations with rooted class metadata. */
#include "inheritance.hpp"
#include "inheritance_constants.h"
#include "flashvm/value.h"
#include <new>

namespace
{
/* [GC:] Stack-owned roots remain usable while finalizers query class identity. */
class ClassRoots
{
    FVM_Context *context_;                 /* [GC:] Context whose active root chain is extended. */
    FVM_GCRootFrame frame_;                /* [GC:] Frame linking this scope to its caller roots. */
    fvm_detail::vector<FVM_Value> values_; /* [GC:] Object values retained by this operation. */
    /* Prevent copying a root-chain owner. */
    ClassRoots(const ClassRoots &);
    /* Prevent assignment that would unbalance active root frames. */
    ClassRoots &operator=(const ClassRoots &);

  public:
    /* [GC:] Attach this frame without allocating a public host root scope. */
    explicit ClassRoots(FVM_Context *c) : context_(c)
    {
        /* [GC:] Do not attach roots during context destruction or resource release. */
        if (!c || c->destroying || c->gc_releasing)
        {
            context_ = 0;
            return;
        }
        /* [GC:] Save the caller chain before publishing our construction values. */
        frame_.previous = c->gc_frames;
        /* [GC:] Trace all values appended to this local vector. */
        frame_.stack = &values_;
        /* [GC:] This scope contributes values rather than an activation binding map. */
        frame_.locals = 0;
        /* [GC:] Receiver objects are explicitly added to the value vector. */
        frame_.receiver = 0;
        /* [GC:] Make this frame visible to callback-triggered collection. */
        c->gc_frames = &frame_;
    }
    /* [GC:] Restore the caller roots on normal and exceptional exits. */
    ~ClassRoots()
    {
        if (context_)
            /* [GC:] Remove the frame before its vector storage is destroyed. */
            context_->gc_frames = frame_.previous;
    }
    /* [GC:] Retain an owned object while callbacks inspect or mutate class metadata. */
    FVM_Result add(FVM_Object *object)
    {
        if (!context_ || !object || object->owner != context_)
            return FVM_INVALID_ARGUMENT;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        /* [GC:] The live frame traces this value even during collector finalization. */
        values_.push_back(value);
        return FVM_OK;
    }
};
/* Validate a constructor and read its prototype without invoking the constructor. */
FVM_Result prototype(FVM_Context *c, FVM_Object *ctor, FVM_Object **out)
{
    if (!c || !ctor || ctor->owner != c || !fvm_is_class_constructor(ctor))
        return FVM_INVALID_ARGUMENT;
    FVM_Value value;
    FVM_Result result = fvm_object_get(c, ctor, "prototype", &value);
    if (result != FVM_OK)
        return result;
    if (value.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    *out = value.object;
    return FVM_OK;
}
} // namespace

/* Accept the same constructor kinds as instanceof, including native classes. */
bool fvm_is_class_constructor(const FVM_Object *constructor)
{
    return constructor && fvm_class_kind(constructor->resource_kind);
}

/* Establish prototype inheritance and superclass metadata without constructor side effects. */
FVM_Result fvm_class_extend(FVM_Context *c, FVM_Object *subclass, FVM_Object *superclass)
{
    if (!c || !subclass || !superclass || subclass == superclass || subclass->owner != c ||
        superclass->owner != c || !fvm_is_class_constructor(subclass))
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Protect both constructors and the unpublished replacement prototype. */
        ClassRoots roots(c);
        FVM_Result result = roots.add(subclass);
        if (result == FVM_OK)
            /* [GC:] Retain superclass through any subsequent property callback. */
            result = roots.add(superclass);
        if (result != FVM_OK)
            return result;
        FVM_Object *parent = 0, *child = 0;
        result = prototype(c, superclass, &parent);
        if (result == FVM_OK)
            /* [GC:] Retain parent through any subsequent property callback. */
            result = roots.add(parent);
        if (result == FVM_OK)
            result = fvm_object_create(c, &child);
        if (result == FVM_OK)
            /* [GC:] Retain child through any subsequent property callback. */
            result = roots.add(child);
        if (result != FVM_OK)
            return result;
        result = fvm_object_set_prototype(c, child, parent);
        if (result != FVM_OK)
            return result;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = superclass;
        result = fvm_object_set(c, child, "constructor", &value);
        if (result == FVM_OK)
            result = fvm_object_set(c, child, "__constructor__", &value);
        if (result != FVM_OK)
            return result;
        child->attributes["constructor"] = 1;
        child->attributes["__constructor__"] = 1;
        value.object = child;
        return fvm_object_set(c, subclass, "prototype", &value);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Install a validated interface list atomically; inherited interface lists remain on parents. */
FVM_Result fvm_class_implements(FVM_Context *c, FVM_Object *ctor, unsigned int count,
                                const FVM_Value *interfaces)
{
    if (!c || !ctor || (count && !interfaces) || count > FVM_EXEC_MAX_INTERFACE_ARGUMENTS)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] The private list holds constructors, not interface prototype snapshots. */
        ClassRoots roots(c);
        FVM_Result result = roots.add(ctor);
        if (result != FVM_OK)
            return result;
        fvm_detail::vector<FVM_Object *> validated;
        for (unsigned int i = 0; i < count; ++i)
        {
            if (fvm_validate_value(c, interfaces + i) != FVM_OK ||
                interfaces[i].type != FVM_VALUE_OBJECT ||
                !fvm_is_class_constructor(interfaces[i].object))
                return FVM_INVALID_ARGUMENT;
            /* [GC:] Retain interfaces[i].object through any subsequent property callback. */
            result = roots.add(interfaces[i].object);
            if (result != FVM_OK)
                return result;
            validated.push_back(interfaces[i].object);
        }
        FVM_Object *target = 0;
        result = prototype(c, ctor, &target);
        if (result != FVM_OK)
            return result;
        /* [GC:] Publish all retained interface edges only after validation succeeds. */
        target->implemented_interfaces.swap(validated);
        target->interfaces_are_prototypes = false; /* Host API retains replaceable constructor edges. */
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Evaluate class and transitive interface membership with a cycle-safe work list. */
FVM_Result fvm_class_instance_of(FVM_Context *c, const FVM_Value *value, FVM_Object *constructor,
                                 int *out)
{
    *out = 0;
    if (value->type != FVM_VALUE_OBJECT)
        return FVM_OK;
    try
    {
        /* [GC:] Property callbacks may detach nodes that are still in the work list. */
        ClassRoots roots(c);
        FVM_Result result = roots.add(value->object);
        if (result == FVM_OK)
            /* [GC:] Retain constructor through any subsequent property callback. */
            result = roots.add(constructor);
        if (result != FVM_OK)
            return result;
        FVM_Object *target = 0;
        result = prototype(c, constructor, &target);
        if (result == FVM_OK)
            /* [GC:] Retain target through any subsequent property callback. */
            result = roots.add(target);
        if (result != FVM_OK)
            return result;
        fvm_detail::vector<FVM_Object *> pending, visited;
        if (value->object->prototype)
            pending.push_back(value->object->prototype);
        while (!pending.empty())
        {
            FVM_Object *node = pending.back();
            pending.pop_back();
            bool seen = false;
            for (size_t i = 0; i < visited.size(); ++i)
                if (visited[i] == node)
                {
                    seen = true;
                    break;
                }
            if (seen)
                continue;
            if (visited.size() >= FVM_CLASS_MAX_GRAPH_NODES)
                return FVM_UNSUPPORTED;
            visited.push_back(node);
            /* [GC:] Retain each visited prototype through any interface prototype callback. */
            /* [GC:] Retain node through any subsequent property callback. */
            result = roots.add(node);
            if (result != FVM_OK)
                return result;
            if (node == target)
            {
                *out = 1;
                return FVM_OK;
            }
            if (node->prototype)
                pending.push_back(node->prototype);
            /* [GC:] Snapshot and root interface references before callbacks can mutate the list. */
            fvm_detail::vector<FVM_Object *> declared = node->implemented_interfaces;
            for (size_t i = 0; i < declared.size(); ++i)
            {
                /* [GC:] Retain declared[i] through any subsequent property callback. */
                result = roots.add(declared[i]);
                if (result != FVM_OK)
                    return result;
            }
            for (size_t i = 0; i < declared.size(); ++i)
            {
                if (node->interfaces_are_prototypes)
                {
                    pending.push_back(declared[i]);
                    continue;
                }
                if (declared[i] == constructor)
                {
                    *out = 1;
                    return FVM_OK;
                }
                FVM_Object *interface_prototype = 0;
                result = prototype(c, declared[i], &interface_prototype);
                if (result != FVM_OK)
                    return result;
                /* [GC:] Retain interface_prototype through any subsequent property callback. */
                result = roots.add(interface_prototype);
                if (result != FVM_OK)
                    return result;
                pending.push_back(interface_prototype);
            }
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
