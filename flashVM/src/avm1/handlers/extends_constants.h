/* FlashVM: Extends operands, boxed classes and the unpublished replacement prototype. */
#ifndef FVM_EXTENDS_CONSTANTS_H
#define FVM_EXTENDS_CONSTANTS_H /* Extends service root-layout guard. */
#define FVM_EXTENDS_SUBCLASS_ROOT                                                                  \
    0U /* [GC:] Original subclass operand during superclass boxing.                                \
        */
#define FVM_EXTENDS_SUPERCLASS_ROOT                                                                \
    1U /* [GC:] Original superclass operand during subclass boxing. */
#define FVM_EXTENDS_BOXED_SUPER_ROOT 2U /* [GC:] Superclass receiver, boxed before subclass. */
#define FVM_EXTENDS_BOXED_SUB_ROOT                                                                 \
    3U                             /* [GC:] Subclass receiver used for final property assignment. */
#define FVM_EXTENDS_PARENT_ROOT 4U /* [GC:] Value returned by superclass.prototype getter. */
#define FVM_EXTENDS_CHILD_ROOT 5U  /* [GC:] New prototype retained through subclass setters. */
#define FVM_EXTENDS_ROOT_COUNT 6U  /* [GC:] Original operands and prepared intermediate values. */
#define FVM_EXTENDS_RAW_PARENT_NAME                                                                \
    "__proto__" /* Own raw inheritance field on the new prototype. */
#define FVM_EXTENDS_CONSTRUCTOR_NAME "constructor" /* Own nonenumerable superclass reference. */
#define FVM_EXTENDS_SUPER_CONSTRUCTOR_NAME "__constructor__" /* AS2 superclass reference. */
#endif
