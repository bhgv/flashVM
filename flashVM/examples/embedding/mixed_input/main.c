/* SDK-free mixed input tutorial; see INPUT_QUEUE.md for capture, cancellation and batch contracts. */
#include "flashvm/input_queue.h" /* Copy Mouse/Key events, then drain on the owning VM thread. */
#include <stdio.h> /* Report results and ordinary API errors. */
#include <string.h> /* Initialize the complete copied keyboard snapshot. */
/* Acquire one VM, deliver one ordered batch and clean partial failures normally. */
int main(void) /* No SDL, Clutter, OS input or physical keyboard is needed. */
{ /* Native storage stays live until all synchronous calls return. */
    FVM_Config config; /* Initialized before runtime acquisition. */
    FVM_Runtime *runtime = 0; /* Owned outer VM, released last. */
    FVM_Context *context = 0; /* Owns the mixed FIFO and pointer/key polling state. */
    FVM_KeyboardState key; /* Copied into the queue, never retained by pointer. */
    FVM_KeyboardState seen; /* Writable native output with exact ABI size. */
    FVM_Result status; /* Preserve the first failure. */
    unsigned int delivered = 0, pending = 0; /* Count successful deliveries and pending copies. */
    double x = 0, y = 0; /* World-coordinate readback after the first batch. */
    memset(&key, 0, sizeof(key)); /* All held/toggle slots must start as booleans. */
    key.struct_size = sizeof(key); /* Required public snapshot ABI size. */
    key.down[FVM_KEY_LETTER_A] = 1; /* Synthetic A-down state, independent of a physical device. */
    key.last_code = FVM_KEY_LETTER_A; /* Latest Flash-style virtual code. */
    key.last_ascii = 'A'; /* ASCII character, not IME composition. */
    seen.struct_size = sizeof(seen); /* Initialize required output ABI field before querying. */
    status = fvm_config_init(&config); /* No runtime exists if this fails. */
    if (status == FVM_OK) /* Acquire the outer owner before its context. */
        status = fvm_runtime_create(&config, &runtime); /* Return owned VM through runtime. */
    if (status == FVM_OK) /* Create input state on this thread only. */
        status = fvm_context_create(runtime, &context); /* No script objects escape, no GC pins needed. */
    if (status == FVM_OK) /* Capture native events after converting their coordinates to world pixels. */
        status = fvm_input_enqueue_mouse(context, FVM_MOUSE_EVENT_DOWN, 12, 18); /* Copy contact begin. */
    if (status == FVM_OK) /* Preserve key and pointer order in the same queue. */
        status = fvm_input_enqueue_keyboard(context, &key, 1, 30, 40); /* Copy full state and event-time pointer. */
    memset(key.down, 0, sizeof(key.down)); /* Changing the producer snapshot cannot change the queued copy. */
    if (status == FVM_OK) /* Enqueue the end even if it arrived before the next frame/tick. */
        status = fvm_input_enqueue_mouse(context, FVM_MOUSE_EVENT_UP, 60, 70); /* Copy contact end. */
    if (status == FVM_OK) /* Deliver only the first two entries on the VM owner thread. */
        status = fvm_input_pump(context, 2, &delivered); /* Publishes key-event position after initial down. */
    if (status == FVM_OK) /* Query committed state, not the last producer's local snapshot. */
        status = fvm_input_get_keyboard(context, &seen); /* Read the copied A-down state. */
    if (status == FVM_OK) /* The keyboard event also restores its saved world pointer. */
        status = fvm_input_get_pointer(context, &x, &y); /* Expect 30,40, not the still-pending up at 60,70. */
    if (status == FVM_OK) /* Read count across all variants without delivering anything. */
        status = fvm_input_pending(context, &pending); /* One up event remains. */
    if (status == FVM_OK && (delivered != 2 || pending != 1 || /* Ordinary tutorial validation. */
        !seen.down[FVM_KEY_LETTER_A] || x != 30 || y != 40)) /* Require independent capture and correct order. */
        status = FVM_ERROR; /* Do not present wrong state as a successful example. */
    if (status == FVM_OK) /* Demonstrate host-chosen cancellation instead of dispatching the pending up. */
        status = fvm_input_clear_queue(context); /* No callback, no synthetic up, no polling reset. */
    if (status == FVM_OK) /* The host separately chooses how to reset held keyboard state on focus loss. */
        status = fvm_input_reset_keyboard(context); /* This operation is independent of FIFO cancellation. */
    if (status == FVM_OK) /* Verify cancellation left no deliverable entries. */
        status = fvm_input_pump(context, 0, &delivered); /* Empty entry batch: no callbacks or VM input changes. */
    if (status == FVM_OK && delivered) /* An empty queue must deliver nothing. */
        status = FVM_ERROR; /* Use ordinary error handling, not tutorial assertions. */
    if (status == FVM_OK) /* Print the verified result before destroying its context. */
        printf("mixed input: pointer=30,40 key snapshot copied; pending event cancelled\n"); /* Expected output. */
    if (context) /* No callback is active during cleanup. */
        fvm_context_destroy(context); /* Release the queue and input state first. */
    if (runtime) /* Every acquired context is gone. */
        fvm_runtime_destroy(runtime); /* Release the outer owner last. */
    if (status != FVM_OK) /* Preserve meaningful VM errors for the caller. */
        fprintf(stderr, "mixed input failed: %d\n", (int)status); /* No unavailable SDK is hidden as success. */
    return status == FVM_OK ? 0 : 1; /* Native stack storage expires after all synchronous calls. */
} /* [GC:] There were no borrowed managed object pointers requiring explicit roots. */
