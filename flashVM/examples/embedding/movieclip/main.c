/* C89 tutorial: bounded stopped MovieClip ticks and committed callback effects; see README.md. */
#include "flashvm/movieclip.h" /* Create a timeline and drive its owner-thread ticks explicitly. */
#include "flashvm/execution.h" /* Execute selected frame actions without advancing the playhead; see FRAME_BLOCKS.md. */
#include "flashvm/standard.h" /* Install the MovieClip class and runtime globals. */
#include "flashvm/gc.h" /* Pin the host-owned root between calls; context destruction releases it. */
#include "tutorial_limits.h" /* Name this tutorial's deliberate work limit and batch size. */
#include <stdio.h> /* Report ordinary errors and deterministic callback counts. */
/* Borrow the counter until context destruction; this callback owns no managed capture. */
static FVM_Result tick(FVM_Context *context, FVM_Object *receiver, unsigned int count, const FVM_Value *args, FVM_Value *out, void *raw) /* See README.md for callback ownership. */
{ /* Execute synchronously inside the native tick barrier. */
    unsigned int *calls = (unsigned int *)raw; /* The host keeps this counter alive throughout cleanup. */
    (void)context; (void)receiver; (void)count; (void)args; /* No script arguments or VM operations are needed. */
    ++*calls; /* Preserve this effect even if a later tick exceeds its budget. */
    fvm_value_undefined(out); /* Optional event delivery ignores the result value. */
    return FVM_OK; /* Permit the bounded traversal to continue. */
} /* Do not destroy or access the context from another thread here. */
/* One stack-owned log outlives every native method registration in the second context. */
struct DepthLog /* Share ordinary C storage, never managed or serialized pointers. */
{ /* Bound observations independently of callback order. */
    unsigned int digits[FVM_TUTORIAL_DEPTH_CHILDREN]; /* Store the low/middle/high identities. */
    unsigned int count; /* Count committed callbacks. */
}; /* End the borrowed host log. */
/* Each registration borrows one identity entry until its context is destroyed. */
struct DepthEntry /* Keep callback identity independent of the native child hash table. */
{ /* A registration owns no C allocation. */
    struct DepthLog *log; /* Borrow the enclosing demonstration's stack-owned log. */
    unsigned int digit; /* Expected ascending-order identity: low=1, middle=2, high=3. */
}; /* End the native registration payload. */
/* Record the actual child order without allocating or advancing another timeline. */
static FVM_Result depth_tick(FVM_Context *context, FVM_Object *receiver, unsigned int count, const FVM_Value *args, FVM_Value *out, void *raw) /* Called synchronously on the owner thread. */
{ /* The enclosing tick barrier retains receiver while this userdata is borrowed. */
    struct DepthEntry *entry = (struct DepthEntry *)raw; /* The host keeps this entry alive until cleanup. */
    (void)context; (void)receiver; (void)count; (void)args; /* Only the independent native identity is needed. */
    fvm_value_undefined(out); /* The event route ignores result values. */
    if (entry->log->count >= FVM_TUTORIAL_DEPTH_CHILDREN) /* Detect duplicate delivery before writing the bounded log. */
        return FVM_INVALID_ARGUMENT; /* Preserve completed entries for host inspection. */
    entry->log->digits[entry->log->count++] = entry->digit; /* Commit this receiver's observed position. */
    return FVM_OK; /* Continue the ordered snapshot. */
} /* Finish this native event. */
/* Create children out of depth order in a separate runtime with its own finite work limit. */
static int depth_demo(void) /* No raster backend, file, SDL adapter or SWF compiler is needed. */
{ /* Initialize partial owners before the first fallible operation. */
    FVM_Config config; /* Runtime creation copies this second configuration. */
    FVM_Runtime *runtime = 0; /* Outlive the managed context. */
    FVM_Context *context = 0; /* Own the root and all child registrations. */
    FVM_Object *root = 0; /* Pin while retaining borrowed child identities. */
    FVM_Value args[2], out; /* Temporary construction arguments and borrowed child results. */
    struct DepthLog log = {{0,0,0},0}; /* Keep native observation storage alive through context destruction. */
    struct DepthEntry entries[FVM_TUTORIAL_DEPTH_CHILDREN]; /* Borrow one stable entry per registration. */
    const char *names[FVM_TUTORIAL_DEPTH_CHILDREN] = {"high","low","middle"}; /* Deliberately shuffled insertion. */
    const long depths[FVM_TUTORIAL_DEPTH_CHILDREN] = {10,-5,2}; /* Ascending traversal differs from insertion order. */
    const unsigned int digits[FVM_TUTORIAL_DEPTH_CHILDREN] = {3,1,2}; /* Independent expected identities. */
    unsigned char pixels[FVM_TUTORIAL_RASTER_BYTES] = {0}; /* Caller-owned transparent RGBA storage for empty geometry. */
    FVM_RasterSurface surface = {0, FVM_TUTORIAL_RASTER_SIDE, FVM_TUTORIAL_RASTER_SIDE, FVM_TUTORIAL_RASTER_STRIDE}; /* Borrow storage only during synchronous rendering. */
    unsigned int i; /* C89 loop counter declared before statements. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the complete configuration ABI. */
    surface.pixels = pixels; /* Bind the automatic array after C89 declarations finish. */
    if (status == FVM_OK) /* Use a separate finite bound for sorting and child delivery. */
    { /* This does not alter the earlier five-unit demonstration. */
        config.instruction_limit = FVM_TUTORIAL_DEPTH_WORK_LIMIT; /* Budget is work units, not milliseconds. */
        status = fvm_runtime_create(&config, &runtime); /* No thread or native device is started. */
    } /* Runtime retains its copied configuration. */
    if (status == FVM_OK) /* Acquire the owner-thread managed heap. */
        status = fvm_context_create(runtime, &context); /* All remaining API calls use this context synchronously. */
    if (status == FVM_OK) /* Install native MovieClip methods before constructing the tree. */
        status = fvm_standard_install(context); /* Preserve errors for cleanup. */
    if (status == FVM_OK) /* Create one frame with no root handler. */
        status = fvm_movieclip_create(context, "ordered", 1, &root); /* The context owns the returned borrowed root. */
    if (status == FVM_OK) /* Retain the complete scene across host boundaries. */
        status = fvm_gc_pin(context, root); /* [GC:] Context destruction releases the pin. */
    for (i = 0; status == FVM_OK && i < FVM_TUTORIAL_DEPTH_CHILDREN; ++i) /* Preserve the intentionally shuffled insertion order. */
    { /* Fully configure one child before constructing the next. */
        entries[i].log = &log; /* Borrow the same stack-owned trace. */
        entries[i].digit = digits[i]; /* Keep identity separate from traversal position. */
        status = fvm_value_string(args, names[i]); /* Prepare a static child name without transferring its C storage. */
        fvm_value_number(args + 1, (double)depths[i]); /* Native public depths may be negative. */
        if (status == FVM_OK) /* Construct through the public optional method bridge. */
            status = fvm_object_call(context, root, "createEmptyMovieClip", 2, args, &out); /* The root retains a successful child. */
        if (status == FVM_OK && (out.type != FVM_VALUE_OBJECT || !out.object)) /* Validate the borrowed method result. */
            status = FVM_INVALID_ARGUMENT; /* Avoid registering on a missing receiver. */
        if (status == FVM_OK) /* No collection happens between construction and registration. */
            status = fvm_object_define_method_rooted(context, out.object, "onEnterFrame", depth_tick, entries + i, 0); /* Stack userdata remains valid until cleanup. */
    } /* Parent graph owns every successfully constructed child. */
    if (status == FVM_OK) /* Snapshot children after the parent tick and visit ascending depths. */
        status = fvm_movieclip_advance(context, root, 1); /* Expected identities are 1,2,3 rather than 3,1,2. */
    if (status == FVM_OK && (log.count != FVM_TUTORIAL_DEPTH_CHILDREN || log.digits[0] != 1 || log.digits[1] != 2 || log.digits[2] != 3)) /* Verify observed identities, not table iteration. */
        status = FVM_INVALID_ARGUMENT; /* Preserve mismatches through ordinary cleanup. */
    if (status == FVM_OK) /* Report only verified observations. */
        puts("depth order: low middle high"); /* Expected third output line of the complete tutorial. */
    if (status == FVM_OK) /* Rendering visits sorted children but never dispatches onEnterFrame. */
        status = fvm_movieclip_render(context, root, &surface); /* Empty geometry works without a raster backend; real paths require one. */
    if (status == FVM_OK && log.count != FVM_TUTORIAL_DEPTH_CHILDREN) /* Preserve the three tick callbacks exactly. */
        status = FVM_INVALID_ARGUMENT; /* Rendering must not manufacture another timeline tick. */
    if (status == FVM_OK) /* Report the verified host rendering boundary. */
        puts("render: no timeline callbacks"); /* Fourth deterministic tutorial output line. */
    else /* A failed tick/render can already have committed native effects. */
        fprintf(stderr, "MovieClip depth tutorial failed: %d\n", (int)status); /* Do not replay the event automatically. */
    if (context) /* Release registrations before the borrowed entries/log go out of scope. */
        fvm_context_destroy(context); /* [GC:] No managed pointer is used after return. */
    if (runtime) /* Release the outer owner last. */
        fvm_runtime_destroy(runtime); /* No thread/device cleanup remains. */
    return status == FVM_OK; /* Preserve failure for the caller's exit code. */
} /* End the independent depth-order demonstration. */
/* Execute frame two twice with a fresh five-unit budget per outer host call; see README.md. */
static int frame_call_demo(void) /* Public C89 execution service needs no SWF file or renderer. */
{ /* Declare all C89 locals before acquiring owners. */
    FVM_Config config; /* Each outer frame call uses the copied runtime limit. */
    FVM_Runtime *runtime = 0; /* Outlive the context and all callback registrations. */
    FVM_Context *context = 0; /* Own the frame definitions and managed clip. */
    FVM_Object *clip = 0; /* Pin before retaining this borrowed pointer between calls. */
    FVM_Value argument, seen, playhead; /* Caller-owned scalar inputs and borrowed property results. */
    unsigned int calls = 0; /* Borrow this counter until context destruction. */
    const unsigned char code[] = { /* Raw independent AVM1: called = 42, then End. */
        0x96,8,0,0,'c','a','l','l','e','d',0, /* Push the variable name as a counted action payload. */
        0x96,5,0,7,42,0,0,0, /* Push integer 42 using the AVM1 integer tag. */
        0x1d,0 /* SetVariable commits the value; End terminates this exact frame block. */
    }; /* The definition API copies and validates these stack-owned bytes. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the complete configuration ABI. */
    if (status == FVM_OK) /* Keep this demonstration's budget independent of tick/depth limits. */
    { /* No device or background worker is started. */
        config.instruction_limit = FVM_TUTORIAL_FRAME_CALL_WORK_LIMIT; /* One block entry plus four AVM1 instructions. */
        status = fvm_runtime_create(&config, &runtime); /* Runtime owns its copied configuration. */
    } /* The host retains the runtime until the context is destroyed. */
    if (status == FVM_OK) /* Acquire the owner-thread managed heap. */
        status = fvm_context_create(runtime, &context); /* All remaining calls are synchronous on this thread. */
    if (status == FVM_OK) /* Install native MovieClip methods and globals. */
        status = fvm_standard_install(context); /* Preserve failure for ordinary cleanup. */
    if (status == FVM_OK) /* Frame two differs from the initial playhead at frame one. */
        status = fvm_movieclip_create(context, "frame_calls", 2, &clip); /* The returned pointer is context-owned. */
    if (status == FVM_OK) /* Retain the graph across separate host calls. */
        status = fvm_gc_pin(context, clip); /* [GC:] Context destruction releases this pin. */
    if (status == FVM_OK) /* A selected frame call must not deliver a tick notification. */
        status = fvm_object_define_method_rooted(context, clip, "onEnterFrame", tick, &calls, 0); /* Stack userdata remains alive until cleanup. */
    if (status == FVM_OK) /* Copy the independent action block into the second frame. */
        status = fvm_movieclip_define_frame(context, clip, 2, "called", code, sizeof(code)); /* Definition does not execute actions. */
    fvm_value_number(&argument, 2); /* Numeric selection needs no label search. */
    if (status == FVM_OK) /* Share block entry and nested bytecode under one outer limit. */
        status = fvm_exec_call_frame(context, clip, &argument); /* Exact five-unit success does not move the playhead. */
    if (status == FVM_OK) /* A later independent outer call starts its own five-unit limit. */
        status = fvm_exec_call_frame(context, clip, &argument); /* No automatic tick or scene-control application occurs. */
    if (status == FVM_OK) /* Inspect the committed script effect. */
        status = fvm_object_get(context, clip, "called", &seen); /* The numeric result needs no retained managed storage. */
    if (status == FVM_OK) /* Verify the playhead independently from selected action execution. */
        status = fvm_object_get(context, clip, "_currentframe", &playhead); /* Reading does not advance the timeline. */
    if (status == FVM_OK && (seen.type != FVM_VALUE_NUMBER || seen.number != 42 || playhead.type != FVM_VALUE_NUMBER || playhead.number != 1 || calls)) /* Check every visible effect. */
        status = FVM_INVALID_ARGUMENT; /* Report mismatches through normal cleanup. */
    if (status == FVM_OK) /* Report only verified observations. */
        puts("frame call: frame 2 actions; playhead 1; callbacks 0"); /* Fifth deterministic tutorial output line. */
    else /* Prior script effects may remain after an execution error. */
        fprintf(stderr, "MovieClip frame-call tutorial failed: %d\n", (int)status); /* Never replay a failed call automatically. */
    if (context) /* Release the graph and callbacks before their stack userdata expires. */
        fvm_context_destroy(context); /* [GC:] No managed pointer is used after this returns. */
    if (runtime) /* Release the outer owner last. */
        fvm_runtime_destroy(runtime); /* No asynchronous native work remains. */
    return status == FVM_OK; /* Preserve failure for the main program. */
} /* End the independent frame-action demonstration. */
/* Completed DoInitAction tables must not consume the later tick batches' work budget; see README.md. */
static int initializer_demo(void) /* Raw SWF8 fixture requires no compiler, renderer or device. */
{ /* Keep all C89 declarations before executable statements. */
    FVM_Config config; /* Each external advance shares this runtime's fixed five-unit limit. */
    FVM_Runtime *runtime = 0; /* Outlive the context. */
    FVM_Context *context = 0; /* Own the imported library, code and clip. */
    FVM_Object *root = 0; /* Pin the borrowed graph across host calls. */
    unsigned int calls = 0; /* Count only ordinary onEnterFrame callbacks. */
    const unsigned char swf[] = { /* Independent serialized SWF8: empty sprite plus one End-only initializer. */
        'F','W','S',8,33,0,0,0, /* Uncompressed header and complete 33-byte file length. */
        8,0,0,12,1,0, /* Minimal RECT, 12 fps, one root frame. */
        0xc8,9,1,0,1,0,0x40,0,0,0, /* DefineSprite id 1, one empty sprite frame and End. */
        0xc3,14,1,0,0, /* DoInitAction for sprite id 1: ActionEnd only. */
        0x40,0,0,0 /* Root ShowFrame and End tags. */
    }; /* Loading copies retained bytes and executes no initializer. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the public configuration ABI. */
    if (status == FVM_OK) /* Keep this budget separate from the earlier independent runtimes. */
    { /* The first tick uses node, capture, entry, ActionEnd and optional-handler units. */
        config.instruction_limit = FVM_TUTORIAL_INIT_WORK_LIMIT; /* Five work units, never milliseconds. */
        status = fvm_runtime_create(&config, &runtime); /* No worker or device is created. */
    } /* Runtime retains a copy of config. */
    if (status == FVM_OK) /* Acquire the managed owner-thread heap. */
        status = fvm_context_create(runtime, &context); /* Use this thread for every following call. */
    if (status == FVM_OK) /* Install native MovieClip methods before importing its timeline. */
        status = fvm_standard_install(context); /* Preserve errors for normal cleanup. */
    if (status == FVM_OK) /* Retain a deferred initializer and a single empty root frame. */
        status = fvm_movieclip_load_swf(context, swf, sizeof(swf), "initializers", &root); /* No code runs during parsing. */
    if (status == FVM_OK) /* Retain library/root cycles across outer host calls. */
        status = fvm_gc_pin(context, root); /* [GC:] Context destruction releases this pin. */
    if (status == FVM_OK) /* Observe ordinary ticks independently from initializer work. */
        status = fvm_object_define_method_rooted(context, root, "onEnterFrame", tick, &calls, 0); /* Borrow the stack counter until cleanup. */
    if (status == FVM_OK) /* Attempt the initializer once, then enter the empty root frame. */
        status = fvm_movieclip_advance(context, root, 1); /* Exactly five units; one ordinary callback. */
    if (status == FVM_OK) /* A completed init table contributes no copying/entry units. */
        status = fvm_movieclip_advance(context, root, FVM_TUTORIAL_INIT_LATER_TICKS); /* Two later ticks use four units within the same five-unit limit. */
    if (status == FVM_OK && calls != 3) /* Verify all three ordinary callbacks completed. */
        status = FVM_INVALID_ARGUMENT; /* Preserve an unexpected result for cleanup and exit. */
    if (status == FVM_OK) /* Report only verified work-budget behavior. */
        puts("initializers: next two ticks fit five units"); /* Sixth deterministic tutorial output line. */
    else /* Earlier initialization/callback effects may remain after a failure. */
        fprintf(stderr, "MovieClip initializer tutorial failed: %d\n", (int)status); /* Do not replay a failed batch automatically. */
    if (context) /* Release registrations before borrowed callback storage expires. */
        fvm_context_destroy(context); /* [GC:] No managed pointer is used after return. */
    if (runtime) /* Release the outer owner last. */
        fvm_runtime_destroy(runtime); /* No asynchronous work remains. */
    return status == FVM_OK; /* Preserve demonstration failure for main. */
} /* End the independent initializer-work example. */
/* Clear a native instance at an owner safe point; no ActionScript callback is active. */
static int unload_demo(void) /* See MOVIE_UNLOAD.md for the atomic publication contract. */
{ /* Initialize every owner before the first fallible call. */
    FVM_Config config; /* Runtime creation copies this host descriptor. */
    FVM_Runtime *runtime = 0; /* Outlive the context and all managed objects. */
    FVM_Context *context = 0; /* Own the clip graph and its pin. */
    FVM_Object *clip = 0; /* Borrow the instance until context destruction. */
    FVM_Value value; /* Carry a scalar own property across content replacement. */
    int attached = 0; /* Receive the physical attachment query only on success. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the public configuration ABI. */
    if (status == FVM_OK) /* Keep partial setup safe for ordinary cleanup. */
        status = fvm_runtime_create(&config, &runtime); /* Create the independent runtime. */
    if (status == FVM_OK) /* The context requires a live runtime. */
        status = fvm_context_create(runtime, &context); /* Create owner-thread managed storage. */
    if (status == FVM_OK) /* Install the native MovieClip class. */
        status = fvm_standard_install(context); /* No renderer or network provider is needed. */
    if (status == FVM_OK) /* Create a three-frame content graph. */
        status = fvm_movieclip_create(context, "retained", 3, &clip); /* Borrow the resulting instance. */
    if (status == FVM_OK) /* [GC:] Retain the instance through subsequent host calls. */
        status = fvm_gc_pin(context, clip); /* Context destruction releases this host pin. */
    fvm_value_number(&value, 42); /* Keep ordinary instance data independent of its contents. */
    if (status == FVM_OK) /* Publish the property before unloading. */
        status = fvm_object_set(context, clip, "kept", &value); /* Copy the scalar into the instance. */
    if (status == FVM_OK) /* No VM/native event callback is active here. */
        status = fvm_movieclip_unload(context, clip); /* Atomically publish stopped empty contents. */
    if (status == FVM_OK) /* Query identity through native edges rather than script getters. */
        status = fvm_movieclip_attached_to(context, clip, clip, &attached); /* A live root includes itself. */
    if (status == FVM_OK) /* Read back the preserved own property. */
        status = fvm_object_get(context, clip, "kept", &value); /* The scalar needs no managed lifetime. */
    if (status == FVM_OK && (!attached || value.type != FVM_VALUE_NUMBER || value.number != 42)) /* Validate results. */
        status = FVM_ERROR; /* Propagate unexpected state through ordinary cleanup. */
    if (status == FVM_OK) /* Report the deterministic successful result. */
        puts("unload: instance and property retained"); /* No synthetic onUnload event is generated. */
    else /* Keep diagnostics independent of managed storage. */
        fprintf(stderr, "unload tutorial failed: %d\n", (int)status); /* Preserve failure for the caller. */
    if (context) /* [GC:] Destroy the graph and release its pin first. */
        fvm_context_destroy(context); /* No borrowed managed pointer is used afterwards. */
    if (runtime) /* Release the enclosing owner last. */
        fvm_runtime_destroy(runtime); /* No independent audio or transport was started. */
    return status == FVM_OK; /* Let the enclosing example propagate failure. */
} /* End the standalone native content-clear demonstration. */
/* Demonstrate partial completion and the fresh budget of a later outer host call. */
int main(void) /* Build with the independent FlashVM component's examples option. */
{ /* Set partial owners before any fallible acquisition. */
    FVM_Config config; /* Runtime creation copies the execution limit. */
    FVM_Runtime *runtime = 0; /* Outlive the context. */
    FVM_Context *context = 0; /* Own the stopped clip and callback registration. */
    FVM_Object *clip = 0; /* Pin before a host collector can run. */
    FVM_Value ignored; /* Receive the result of stop(). */
    unsigned int calls = 0; /* Borrowed callback userdata remains valid until destruction. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the complete configuration ABI. */
    if (status == FVM_OK) /* Use a small limit to make partial completion deterministic. */
    { /* Change only the ordinary instruction/work bound. */
        config.instruction_limit = FVM_TUTORIAL_TICK_WORK_LIMIT; /* Native traversal and AVM1 callbacks share this budget. */
        status = fvm_runtime_create(&config, &runtime); /* No worker or device is created. */
    } /* A runtime owns a copy of config. */
    if (status == FVM_OK) /* Acquire the managed heap on this thread. */
        status = fvm_context_create(runtime, &context); /* Use this thread for every following VM call. */
    if (status == FVM_OK) /* Install standard MovieClip methods before construction. */
        status = fvm_standard_install(context); /* Host mode needs no SWF bytes. */
    if (status == FVM_OK) /* A single-frame clip is enough to demonstrate tick handlers. */
        status = fvm_movieclip_create(context, "root", 1, &clip); /* The returned pointer is borrowed from the context. */
    if (status == FVM_OK) /* Retain the graph across host call boundaries. */
        status = fvm_gc_pin(context, clip); /* [GC:] Context destruction releases this root. */
    if (status == FVM_OK) /* Pausing a timeline does not disable its onEnterFrame handler. */
        status = fvm_object_call(context, clip, "stop", 0, 0, &ignored); /* Stop frame advancement, not host ticks. */
    if (status == FVM_OK) /* Register the optional native handler without a managed userdata object. */
        status = fvm_object_define_method_rooted(context, clip, "onEnterFrame", tick, &calls, 0); /* The clip retains its registration. */
    if (status == FVM_OK) /* Request three ticks using a single shared five-unit budget. */
    { /* Accept only the intentional limit error after two completed callbacks. */
        status = fvm_movieclip_advance(context, clip, FVM_TUTORIAL_TICK_BATCH); /* The third node entry commits before its handler budget fails. */
        if (status == FVM_ERROR && calls == 2) /* Partial delivery is documented, not a rollback transaction. */
        { /* A new outer call starts a fresh work budget. */
            puts("bounded batch: 2 callbacks retained"); /* Deterministic first output line. */
            status = fvm_movieclip_advance(context, clip, 1); /* Deliver one more handler on the stopped clip. */
        } /* Do not automatically replay the failed three-tick batch. */
        else /* Any unexpected status or count is a tutorial failure. */
            status = FVM_INVALID_ARGUMENT; /* Report the mismatch through normal cleanup. */
    } /* The callback remains registered while its userdata is alive. */
    if (status == FVM_OK && calls != 3) /* Verify that the second call delivered exactly once. */
        status = FVM_INVALID_ARGUMENT; /* Preserve a failing result for the exit code. */
    if (status == FVM_OK) /* Report the complete demonstration. */
        printf("after next call: %u callbacks\n", calls); /* Expected count is three. */
    else /* Report errors before releasing owners. */
        fprintf(stderr, "MovieClip tutorial failed: %d\n", (int)status); /* Keep diagnostics independent of managed storage. */
    if (context) /* Release the pinned graph and registration before counter storage expires. */
        fvm_context_destroy(context); /* [GC:] No callback occurs after this returns. */
    if (runtime) /* Release the copied configuration last. */
        fvm_runtime_destroy(runtime); /* No external platform resource is owned. */
    if (status == FVM_OK && !depth_demo()) /* Run the independent order example after releasing the first runtime. */
        status = FVM_INVALID_ARGUMENT; /* Propagate the second demonstration's failure. */
    if (status == FVM_OK && !frame_call_demo()) /* Run selected frame actions after the previous independent demos. */
        status = FVM_INVALID_ARGUMENT; /* Propagate frame-call demonstration failure. */
    if (status == FVM_OK && !initializer_demo()) /* Verify completed-library work after the other independent demos. */
        status = FVM_INVALID_ARGUMENT; /* Propagate initializer demonstration failure. */
    if (status == FVM_OK && !unload_demo()) /* Run the independent safe-point content-clear example. */
        status = FVM_ERROR; /* Preserve a failed contract check for the shell. */
    return status == FVM_OK ? 0 : 1; /* Let CTest or a shell observe the result. */
} /* End the standalone host. */
