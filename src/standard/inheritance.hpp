/* FlashVM: internal AVM1 class inheritance and interface graph operations. */
#ifndef FVM_STANDARD_INHERITANCE_HPP
#define FVM_STANDARD_INHERITANCE_HPP /* Include guard for class graph helpers. */
#include "../core/runtime.hpp"

/* Identify constructor representations without following any user properties. */
inline bool fvm_class_kind(unsigned long kind)
{
    return kind == 21 || kind == 23 || kind == 25 || kind == 27 || kind == 29 || kind == 31 ||
           kind == 35 || kind == 33 || kind == 32 || kind == 36 || kind == 41 || kind == 1003 ||
           kind == 1004 || kind == 44 || kind == 45 || kind == 47;
}
/* Recognize supported function and native class constructor representations. */
bool fvm_is_class_constructor(const FVM_Object *constructor);
/* Create a subclass prototype without invoking the superclass constructor. */
FVM_Result fvm_class_extend(FVM_Context *, FVM_Object *, FVM_Object *);
/* [GC:] Retain interface constructors on the implementing class prototype. */
FVM_Result fvm_class_implements(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *);
/* Check class and transitive interface membership, avoiding cycles. */
FVM_Result fvm_class_instance_of(FVM_Context *, const FVM_Value *, FVM_Object *, int *);
#endif
