/* FlashVM: legacy property resolver identity and bounded callback recursion. */
#ifndef FVM_CORE_PROPERTY_RESOLVE_CONSTANTS_H
#define FVM_CORE_PROPERTY_RESOLVE_CONSTANTS_H /* Resolver constants guard. */
/* Raw own property searched without evaluating its getter. */
#define FVM_PROPERTY_RESOLVE_NAME "__resolve"
/* Bound nested resolver callbacks independently of their native/script representation. */
#define FVM_PROPERTY_RESOLVE_MAX_DEPTH 64U
/* A resolver is a function callback, independent of the resolver property's owner. */
#define FVM_PROPERTY_RESOLVE_SUPER_DEPTH 0U
#endif
