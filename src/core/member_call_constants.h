/* FlashVM: superclass-depth rules for loaded named member invocation. */
#ifndef FVM_CORE_MEMBER_CALL_CONSTANTS_H
#define FVM_CORE_MEMBER_CALL_CONSTANTS_H /* Named member call constants guard. */
/* A method found directly on its receiver still starts super at prototype depth one. */
#define FVM_MEMBER_CALL_MIN_SUPER_DEPTH 1U
#endif
