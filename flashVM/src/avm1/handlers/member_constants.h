/* FlashVM: member-operation inputs, converted name and temporary wrapper roots. */
#ifndef FVM_MEMBER_CONSTANTS_H
#define FVM_MEMBER_CONSTANTS_H               /* Member service constants guard. */
#define FVM_MEMBER_OWNER_ROOT 0U             /* [GC:] Original receiver, including primitive metadata. */
#define FVM_MEMBER_NAME_ROOT 1U              /* [GC:] Original name before user string conversion. */
#define FVM_MEMBER_ASSIGNED_ROOT                                                                                       \
    2U                                       /* [GC:] Assigned value retained through conversion/setters.      \
                                  */
#define FVM_MEMBER_TEXT_ROOT 3U              /* [GC:] Converted counted property name. */
#define FVM_MEMBER_BOX_ROOT 4U               /* [GC:] Temporary boxed receiver or bare nullish object. */
#define FVM_MEMBER_ROOT_COUNT 5U             /* [GC:] Inputs and temporary representations. */
#define FVM_MEMBER_META_READ_ARGUMENTS 1U    /* Counted name passed to __index. */
#define FVM_MEMBER_META_WRITE_ARGUMENTS 2U   /* Name and assigned value passed to __newindex. */
#define FVM_MEMBER_META_NAME_ARGUMENT 0U     /* Name position in operation-specific hook arguments. */
#define FVM_MEMBER_META_ASSIGNED_ARGUMENT 1U /* Value position after the name in write hooks. */
#endif
