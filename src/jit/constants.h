/* FlashVM: bounded native compilation and System V x86-64 instruction encodings. */
#ifndef FVM_JIT_CONSTANTS_H
#define FVM_JIT_CONSTANTS_H               /* Native compiler constants guard. */
#define FVM_JIT_MAX_SOURCE_BYTES 1048576U /* Maximum copied/decoded action body. */
#define FVM_JIT_MAX_ACTIONS 65536U        /* Maximum emitted action/checkpoint blocks. */
#define FVM_JIT_MAX_CONSTANTS 65536U      /* Maximum immutable constant records. */
#define FVM_JIT_MAX_STACK 65536U          /* Maximum live rooted runtime operand values. */
#define FVM_JIT_MAX_CODE_BYTES 16777216U  /* Bound also keeps relative jumps in signed32 range. */
#define FVM_JIT_ADDRESS_BYTES 8U          /* Native System V AMD64 address representation. */
#define FVM_JIT_IMMEDIATE_BYTES 4U        /* rel32 and imm32 instruction operand width. */
#define FVM_JIT_BYTE_MASK 255UL           /* Low byte when encoding little-endian words. */
#define FVM_JIT_BYTE_BITS 8U              /* Little-endian word byte stride. */
#define FVM_JIT_EXIT_LABEL 0xffffffffUL   /* Fixup target for the shared native epilogue. */
#define FVM_JIT_POLL_MASK 63UL            /* Match interpreter polling on step numbers 1,65,... */
#define FVM_JIT_PUSH_R12_0 0x41U          /* REX prefix for callee-saved r12. */
#define FVM_JIT_PUSH_R12_1 0x54U          /* push r12; aligns the stack before helper calls. */
#define FVM_JIT_SAVE_ARG_0 0x49U          /* mov r12,rdi: retain invocation across C calls. */
#define FVM_JIT_SAVE_ARG_1 0x89U          /* mov register encoding. */
#define FVM_JIT_SAVE_ARG_2 0xfcU          /* rdi source, r12 destination. */
#define FVM_JIT_ARG_0 0x4cU               /* mov rdi,r12: first System V helper argument. */
#define FVM_JIT_ARG_1 0x89U               /* mov register encoding. */
#define FVM_JIT_ARG_2 0xe7U               /* r12 source, rdi destination. */
#define FVM_JIT_OPERAND 0xbeU             /* mov esi,imm32: semantic helper operand. */
#define FVM_JIT_ADDRESS_0 0x48U           /* movabs rax,address: helper address prefix. */
#define FVM_JIT_ADDRESS_1 0xb8U           /* movabs rax opcode. */
#define FVM_JIT_CALL_0 0xffU              /* call rax opcode prefix. */
#define FVM_JIT_CALL_1 0xd0U              /* call rax register selector. */
#define FVM_JIT_TEST_0 0x85U              /* test eax,eax: helper status or condition. */
#define FVM_JIT_TEST_1 0xc0U              /* eax source/destination. */
#define FVM_JIT_JNZ_0 0x0fU               /* jnz rel32 conditional jump prefix. */
#define FVM_JIT_JNZ_1 0x85U               /* jnz rel32 opcode. */
#define FVM_JIT_JMP 0xe9U                 /* jmp rel32 for branch or successful return. */
#define FVM_JIT_POP_R12_0 0x41U           /* Restore callee-saved r12 prefix. */
#define FVM_JIT_POP_R12_1 0x5cU           /* pop r12. */
#define FVM_JIT_RET 0xc3U                 /* Return the status held in eax. */
/* Native plan control flow, independent of encoded Action identifiers. */
#define FVM_JIT_FALLTHROUGH 0
#define FVM_JIT_JUMP 1
#define FVM_JIT_CONDITIONAL 2
#define FVM_JIT_EXIT 3
/* Rooted operand stack operations. */
#define FVM_JIT_POP 0U
#define FVM_JIT_DUPLICATE 1U
#define FVM_JIT_SWAP 2U
#define FVM_JIT_BINARY_VALUES 2U       /* Operand count for arithmetic, comparisons and swaps. */
#define FVM_JIT_UNARY_VALUES 1U        /* Operand count for return, truth and duplication. */
#define FVM_JIT_ENDBR_0 0xf3U          /* ENDBR64 supports CET indirect branch tracking entry. */
#define FVM_JIT_ENDBR_1 0x0fU          /* ENDBR64 second byte. */
#define FVM_JIT_ENDBR_2 0x1eU          /* ENDBR64 third byte. */
#define FVM_JIT_ENDBR_3 0xfaU          /* ENDBR64 fourth byte. */
#define FVM_JIT_CACHE_ENTRIES 32U      /* Bound positive and negative automatic cache entries. */
#define FVM_JIT_CACHE_BYTES 33554432UL /* Bound total emitted bytes retained by a context. */
#define FVM_JIT_REPLAY_EXCEPTION_ROOT                                                              \
    0U /* [GC:] Original thrown value while retry clears pending state. */
#define FVM_JIT_REPLAY_ROOT_COUNT 1U /* [GC:] Complete replay preservation layout. */
#define FVM_JIT_LOG_LABEL_BYTES 256U /* Maximum default diagnostic label output. */
#endif
