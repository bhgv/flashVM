/* FlashVM: encoded byte widths, signed branches and fixed AVM1 stack operand counts. */
#ifndef FVM_AVM1_OPERAND_CONSTANTS_H
#define FVM_AVM1_OPERAND_CONSTANTS_H /* Operand-layout guard. */
#define FVM_AVM1_BYTE_BITS 8U   /* Bits in each encoded byte, independent of host word width. */
#define FVM_AVM1_WORD_BYTES 2U  /* UI16 operand byte length. */
#define FVM_AVM1_DWORD_BYTES 4U /* UI32 operand and each half of an encoded double. */
#define FVM_AVM1_STRING_BINARY_OPERANDS                                                            \
    2U /* StringAdd/comparisons consume left and right values. */
#define FVM_AVM1_STRING_EXTRACT_OPERANDS 3U /* StringExtract consumes input, start and length. */
#define FVM_AVM1_STRING_EXTRACT_START_OFFSET 2U /* Start lies below the top length operand. */
#define FVM_AVM1_DOUBLE_BYTES 8U                /* Two little-endian words, high word first. */
/* Named construction has a separate layout identity from ordinary invocation. */
#define FVM_AVM1_NEW_OBJECT_FIXED_OPERANDS 2U /* NewObject has count and name above arguments. */
#define FVM_AVM1_NEW_OBJECT_FIRST_ARGUMENT                                                         \
    3U /* Offset below constructor name/count to arg zero.                                         \
        */
#define FVM_AVM1_NAMED_CALL_FIXED_OPERANDS                                                         \
    2U /* CallFunction has count and name above arguments.                                         \
        */
#define FVM_AVM1_NAMED_CALL_FIRST_ARGUMENT                                                         \
    3U /* Offset below name/count to the first call argument. */
#define FVM_AVM1_DECLARE_INITIALIZED_OPERANDS 2U /* DefineLocal consumes name and initializer. */
#define FVM_AVM1_DECLARE_ONLY_OPERANDS 1U        /* DefineLocal2 consumes only its name. */
#define FVM_AVM1_VARIABLE_WRITE_OPERAND_COUNT                                                      \
    2U                                   /* SetVariable consumes name followed by value.           \
                                          */
#define FVM_AVM1_BRANCH_BYTES 2U         /* Signed UI16 relative jump payload length. */
#define FVM_AVM1_BRANCH_SIGN_BIT 0x8000L /* Upper half of the signed branch encoding. */
#define FVM_AVM1_BRANCH_MODULUS 65536L   /* Subtract 2^16 to sign-extend a branch into long. */
#define FVM_AVM1_CAST_OPERANDS 2U        /* CastOp consumes class followed by the original value. */
#define FVM_AVM1_INSTANCE_OF_OPERANDS 2U /* InstanceOf consumes value followed by class. */
#define FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS                                                         \
    2U /* Interface count and class above interface values. */
#define FVM_AVM1_IMPLEMENTS_FIRST_INTERFACE                                                        \
    3U                                      /* Offset below class to the first interface value. */
#define FVM_AVM1_EXTENDS_OPERANDS 2U        /* Extends consumes subclass followed by superclass. */
#define FVM_AVM1_COMPARE_OPERANDS 2U        /* Dynamic comparison and Add2 consume two values. */
#define FVM_AVM1_NUMERIC_BINARY_OPERANDS 2U /* Arithmetic, remainder and bitwise binary values. */
#define FVM_AVM1_GET_MEMBER_OPERANDS 2U     /* GetMember consumes owner and property name. */
#define FVM_AVM1_SET_MEMBER_OPERANDS 3U     /* SetMember additionally consumes a property value. */
#define FVM_AVM1_MEMBER_NAME_OFFSET 1U      /* Property name relative to the first owner operand. */
#define FVM_AVM1_MEMBER_VALUE_OFFSET 2U    /* Property value relative to the first owner operand. */
#define FVM_AVM1_DELETE_MEMBER_OPERANDS 2U /* Delete consumes owner and property name. */
#define FVM_AVM1_SWAP_OPERANDS 2U          /* StackSwap needs the top two values. */
#define FVM_AVM1_LEGACY_BINARY_OPERANDS 2U /* Legacy comparison and logical binary operands. */
/* [GC:] Prepared frame or label retained while resolving the selected timeline. */
#define FVM_AVM1_TIMELINE_REQUEST_ROOT 0U
#define FVM_AVM1_TIMELINE_REQUEST_ROOT_COUNT 1U /* [GC:] Single prepared frame/label value. */
/* Indexed display access consumes a target/index pair, plus a value for writes. */
#define FVM_AVM1_DISPLAY_READ_OPERANDS 2U  /* Target and indexed property selector. */
#define FVM_AVM1_DISPLAY_WRITE_OPERANDS 3U /* Target, selector and assigned value. */
#define FVM_AVM1_DISPLAY_INDEX_OFFSET 1U   /* Index follows the target in prepared operands. */
#define FVM_AVM1_DISPLAY_VALUE_OFFSET 2U   /* Assigned value follows target and index. */
/* [GC:] Original URL request roots, separate from encoded operand and host callback counts. */
#define FVM_AVM1_URL_REQUEST_URL_ROOT 0U    /* [GC:] Original URL through service coercion. */
#define FVM_AVM1_URL_REQUEST_TARGET_ROOT 1U /* [GC:] Original target through URL conversion. */
#define FVM_AVM1_URL_REQUEST_ROOT_COUNT 2U  /* [GC:] Original request pair; no unused third slot.  \
                                             */
#define FVM_AVM1_URL_OPERANDS 2U            /* GetURL2 consumes URL followed by target. */
#define FVM_AVM1_METHOD_FIXED_OPERANDS                                                             \
    3U                                    /* Method call has count, receiver and name above args.  \
                                           */
#define FVM_AVM1_METHOD_OWNER_OFFSET 2U   /* Receiver lies below the method name. */
#define FVM_AVM1_METHOD_FIRST_ARGUMENT 4U /* First argument lies below count/receiver/name. */
/* CloneSprite consumes source, instance name and absolute display depth. */
#define FVM_AVM1_CLONE_OPERANDS 3U /* Full clone request, with loaded missing-pop padding. */
#define FVM_AVM1_CLONE_SOURCE_OFFSET 0U /* Source relative to the first prepared operand. */
#define FVM_AVM1_CLONE_NAME_OFFSET 1U /* New instance name. */
#define FVM_AVM1_CLONE_DEPTH_OFFSET 2U /* Absolute display depth converted first. */
#endif
