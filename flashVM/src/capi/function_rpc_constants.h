/* FlashVM: versioned function-call capability and correlated TCP frame layout. */
#ifndef FVM_FUNCTION_RPC_CONSTANTS_H
#define FVM_FUNCTION_RPC_CONSTANTS_H        /* Function RPC protocol constants guard. */
#define FVM_FUNCTION_CAPABILITY_KIND 9UL    /* Function target token on the shared authority. */
#define FVM_FUNCTION_RPC_VERSION 2U         /* Correlated function envelope, separate from resource v1. */
#define FVM_FUNCTION_RPC_CALL 10U           /* Invoke the original function with a serialized argument frame. */
#define FVM_FUNCTION_RPC_REQUEST_SIZE 144U  /* Legacy routing prefix plus random call ID. */
#define FVM_FUNCTION_RPC_RESPONSE_SIZE 116U /* Echoed target/source/call IDs and result framing. */
#define FVM_FUNCTION_RPC_ID_BYTES 32U       /* ASCII random 128-bit identity. */
#define FVM_FUNCTION_RPC_CALL_ID 112U       /* Request correlation identity offset. */
#define FVM_FUNCTION_RPC_STATUS 108U        /* Response unsigned magnitude of FVM_Result. */
#define FVM_FUNCTION_RPC_LENGTH 112U        /* Response payload byte length. */
#define FVM_FUNCTION_RPC_MAX_DEPTH 32U      /* Bound direct nested authority calls before C stack exhaustion. */
#define FVM_FUNCTION_RPC_MAX_ARGS 65536UL   /* Bounded argument frame entries, excluding receiver. */
#define FVM_FUNCTION_RPC_TIMEOUT_MS 2000UL  /* No retry: timeout leaves remote execution uncertain. */
/* Routing offsets shared by request and response codecs. */
#define FVM_FUNCTION_RPC_VERSION_OFFSET 4U   /* Wire version offset. */
#define FVM_FUNCTION_RPC_OPERATION_OFFSET 5U /* Wire operation offset. */
#define FVM_FUNCTION_RPC_KIND_OFFSET 6U      /* Wire kind offset. */
#define FVM_FUNCTION_RPC_FLAGS_OFFSET 7U     /* Wire flags offset. */
#define FVM_FUNCTION_RPC_SESSION_OFFSET 8U   /* Wire session offset. */
#define FVM_FUNCTION_RPC_TARGET_OFFSET 40U   /* Wire target offset. */
#define FVM_FUNCTION_RPC_SOURCE_OFFSET 72U   /* Wire source offset. */
#define FVM_FUNCTION_RPC_CONTEXT_OFFSET 104U /* Wire context offset. */
#define FVM_FUNCTION_RPC_REQUEST_LENGTH 108U /* Wire request length. */
#define FVM_FUNCTION_RPC_REPLY_TARGET 8U     /* Wire reply target. */
#define FVM_FUNCTION_RPC_REPLY_SOURCE 40U    /* Wire reply source. */
#define FVM_FUNCTION_RPC_REPLY_CONTEXT 72U   /* Wire reply context. */
#define FVM_FUNCTION_RPC_REPLY_CALL 76U      /* Wire reply call. */
#define FVM_FUNCTION_RPC_MAGIC_BYTES 4U      /* Wire magic bytes. */
#define FVM_FUNCTION_RPC_WORD_BYTES 4U       /* Wire word bytes. */
#define FVM_FUNCTION_RPC_ID_STORAGE 33U      /* Wire id storage. */
#define FVM_FUNCTION_RPC_URI_CAPACITY 512U   /* Wire uri capacity. */
#define FVM_FUNCTION_RPC_FAULT_FLAG 1U       /* Wire fault flag. */
#define FVM_FUNCTION_RPC_MAX_RESULT 6UL      /* Wire max result. */
#define FVM_FUNCTION_RPC_RECEIVER_INDEX 0UL  /* Wire receiver index. */
#define FVM_FUNCTION_RPC_ARGUMENT_BASE 1UL   /* Wire argument base. */
/* FVMF v1 diagnostic fields: seven unsigned 64-bit values, then bounded text. */
#define FVM_FUNCTION_RPC_FAULT_SIZE 1776U       /* Wire fault size. */
#define FVM_FUNCTION_RPC_FAULT_NUMBERS 7U       /* Wire fault numbers. */
#define FVM_FUNCTION_RPC_FAULT_HEADER 8U        /* Wire fault header. */
#define FVM_FUNCTION_RPC_FAULT_INTEGER_BYTES 8U /* Wire fault integer bytes. */
#define FVM_FUNCTION_RPC_FAULT_MESSAGE 64U      /* Wire fault message. */
#define FVM_FUNCTION_RPC_FAULT_OPERATION 192U   /* Wire fault operation. */
#define FVM_FUNCTION_RPC_FAULT_KIND 224U        /* Wire fault kind. */
#define FVM_FUNCTION_RPC_FAULT_PATH 240U        /* Wire fault path. */
#define FVM_FUNCTION_RPC_FAULT_ADDRESS 752U     /* Wire fault address. */
#define FVM_FUNCTION_RPC_FAULT_SOURCE 1264U     /* Wire fault source. */
#define FVM_FUNCTION_RPC_FAULT_FUNCTION 1520U   /* Wire fault function. */
#endif
