/* FlashVM tutorial: copy a color matrix into a filter and swap red/blue in one RGBA pixel. */
#include "flashvm/standard.h" /* Declare runtime, object and standard ActionScript operations. */
#include "flashvm/table.h"    /* Declare indexed Array writes for the twenty matrix coefficients. */
#include "flashvm/gc.h" /* [GC:] Declare explicit roots for objects retained in C local variables. */
#include <stdio.h>      /* Declare diagnostic and resulting pixel output. */

/* Make a borrowed object value; the caller keeps its object pinned until context cleanup. */
static FVM_Value reference(FVM_Object *object) /* Accept an object owned by the active context. */
{                                  /* Begin construction of the tagged object argument. */
    FVM_Value value;               /* Hold the value passed to constructors and methods. */
    fvm_value_undefined(&value);   /* Clear the complete value before setting its object payload. */
    value.type = FVM_VALUE_OBJECT; /* Select the managed object variant. */
    value.object = object;         /* [GC:] Borrow the caller's explicitly retained object. */
    return value;                  /* Return a value rather than copying the managed object. */
} /* End object argument construction. */

/* Run a backend-independent filter example, returning failure if any API operation fails. */
int main(void) /* Use a C89-compatible process entry point without command-line parameters. */
{              /* Begin the interpreter and object lifetime scope. */
    FVM_Config config;        /* Hold default runtime options before initialization. */
    FVM_Runtime *runtime = 0; /* Initialize null so cleanup also handles creation failure. */
    FVM_Context *context =
        0; /* [GC:] The context owns all objects and their private filter storage. */
    FVM_Object *array =
        0; /* [GC:] Retain the input coefficient Array with an explicit pin below. */
    FVM_Object *effect = 0; /* [GC:] Retain the constructed ColorMatrixFilter below. */
    FVM_Object *image = 0;  /* [GC:] Retain the one-pixel BitmapData below. */
    FVM_Object *rect = 0;   /* [GC:] Retain the source region Rectangle below. */
    FVM_Object *point = 0;  /* [GC:] Retain the destination Point below. */
    FVM_Value args[4];      /* Reuse four initialized arguments for constructors and applyFilter. */
    FVM_Value result;       /* Receive the filtered ARGB number from getPixel32. */
    double argb;            /* Hold the unsigned 32-bit color as an exact double. */
    FVM_Result status = FVM_OK; /* Save the last API result for the error diagnostic. */
    unsigned
        i; /* Iterate over all twenty coefficients without a declaration inside the C89 loop. */
    int exit_code = 1; /* Assume failure until the expected filtered pixel is verified. */
    const double matrix[20] = {
        /* Specify four output channel rows, each with RGBA weights and offset. */
        0, 0, 1, 0, 0, /* Red output takes the source blue channel without an offset. */
        0, 1, 0, 0, 0, /* Green output preserves source green. */
        1, 0, 0, 0, 0, /* Blue output takes source red. */
        0, 0, 0, 1, 0  /* Alpha output preserves the original alpha. */
    }; /* Finish the fixed 4-by-5 color transform. */

    if ((status = fvm_config_init(&config)) != FVM_OK) /* Initialize default configuration. */
        goto cleanup; /* Report initialization failure without creating a runtime. */
    if ((status = fvm_runtime_create(&config, &runtime)) !=
        FVM_OK)       /* Create the configured runtime. */
        goto cleanup; /* Release any runtime resources already created. */
    if ((status = fvm_context_create(runtime, &context)) != FVM_OK) /* Create its script context. */
        goto cleanup; /* [GC:] Cleanup accepts a null context if allocation failed. */
    if ((status = fvm_standard_install(context)) !=
        FVM_OK)       /* Install Array, geometry, bitmap and filter classes. */
        goto cleanup; /* [GC:] Context destruction releases partially staged managed classes too. */

    fvm_value_number(args, 20); /* Pass Array length twenty as constructor argument zero. */
    if ((status = fvm_construct(context, "Array", 1, args, &array)) !=
        FVM_OK)       /* Allocate the coefficient Array. */
        goto cleanup; /* Abort before writing coefficients if allocation failed. */
    /* [GC:] Establish the host root before the next operation can expose another object. */
    if ((status = fvm_gc_pin(context, array)) !=
        FVM_OK)              /* [GC:] Root the Array while C holds its pointer. */
        goto cleanup;        /* [GC:] Abort if the explicit root could not be established. */
    for (i = 0; i < 20; ++i) /* Visit each row-major coefficient index. */
    {                        /* Begin one indexed coefficient write. */
        fvm_value_number(args,
                         matrix[i]); /* Convert the host double coefficient into an AS number. */
        if ((status = fvm_table_set_index(context, array, i, args)) !=
            FVM_OK)       /* Store that number at Array index i. */
            goto cleanup; /* Preserve the diagnostic if an indexed write fails. */
    } /* Finish filling the coefficient Array. */
    args[0] = reference(array); /* Supply the Array whose twenty entries the filter will copy. */
    if ((status = fvm_construct(context, "flash.filters.ColorMatrixFilter", 1, args, &effect)) !=
        FVM_OK)       /* Construct independent native matrix storage. */
        goto cleanup; /* Abort if coefficient conversion or filter allocation failed. */
    /* [GC:] Establish the host root before the next operation can expose another object. */
    if ((status = fvm_gc_pin(context, effect)) !=
        FVM_OK)       /* [GC:] Retain the filter for applyFilter. */
        goto cleanup; /* [GC:] Abort if the filter root could not be installed. */

    fvm_value_number(args, 1);      /* Set BitmapData width to one pixel. */
    fvm_value_number(args + 1, 1);  /* Set height to one pixel. */
    fvm_value_boolean(args + 2, 1); /* Enable transparent storage so alpha 128 is retained. */
    fvm_value_number(args + 3,
                     0x80c86432UL); /* Initialize ARGB: alpha 128, red 200, green 100, blue 50. */
    if ((status = fvm_construct(context, "flash.display.BitmapData", 4, args, &image)) !=
        FVM_OK)       /* Allocate and fill native pixel storage. */
        goto cleanup; /* Abort if the one-pixel bitmap could not be created. */
    /* [GC:] Establish the host root before the next operation can expose another object. */
    if ((status = fvm_gc_pin(context, image)) !=
        FVM_OK)                    /* [GC:] Retain the bitmap through filtering and output. */
        goto cleanup;              /* [GC:] Abort if its root could not be established. */
    fvm_value_number(args, 0);     /* Start the source Rectangle at column zero. */
    fvm_value_number(args + 1, 0); /* Start it at row zero. */
    fvm_value_number(args + 2, 1); /* Include one source column. */
    fvm_value_number(args + 3, 1); /* Include one source row. */
    if ((status = fvm_construct(context, "flash.geom.Rectangle", 4, args, &rect)) !=
        FVM_OK)       /* Construct the complete one-pixel source region. */
        goto cleanup; /* Abort if region construction fails. */
    /* [GC:] Establish the host root before the next operation can expose another object. */
    if ((status = fvm_gc_pin(context, rect)) !=
        FVM_OK)       /* [GC:] Retain the source region while preparing the call. */
        goto cleanup; /* [GC:] Abort if the Rectangle root could not be established. */
    if ((status = fvm_construct(context, "flash.geom.Point", 0, 0, &point)) !=
        FVM_OK)       /* Create default destination point (0,0). */
        goto cleanup; /* Abort if the destination Point could not be created. */
    /* [GC:] Establish the host root before the next operation can expose another object. */
    if ((status = fvm_gc_pin(context, point)) != FVM_OK) /* [GC:] Retain the destination Point. */
        goto cleanup; /* [GC:] Abort if its explicit root failed. */

    args[0] =
        reference(image); /* Read source pixels from the same BitmapData that receives output. */
    args[1] = reference(rect);   /* Restrict the copied source ROI to its single pixel. */
    args[2] = reference(point);  /* Write the filtered ROI at destination coordinate (0,0). */
    args[3] = reference(effect); /* Use the copied red/blue swap coefficients. */
    if ((status = fvm_object_call(context, image, "applyFilter", 4, args, &result)) !=
        FVM_OK)       /* Snapshot source before modifying overlapping destination pixels. */
        goto cleanup; /* Report filtering or scratch allocation failure. */
    fvm_value_number(args, 0);     /* Select pixel column zero for the output read. */
    fvm_value_number(args + 1, 0); /* Select pixel row zero. */
    if ((status = fvm_object_call(context, image, "getPixel32", 2, args, &result)) !=
        FVM_OK)       /* Read complete filtered ARGB, including alpha. */
        goto cleanup; /* Report pixel lookup failure. */
    argb = result.type == FVM_VALUE_NUMBER ? result.number : 0; /* Read only a numeric payload. */
    if (argb < 0) /* getPixel32 returns a signed 32-bit ARGB number. */
        argb +=
            4294967296.0; /* Normalize the signed result before comparing unsigned color bits. */
    if (result.type != FVM_VALUE_NUMBER ||
        argb != 0x803264c8UL)          /* Expect alpha 128, red 50, green 100, blue 200. */
    {                                  /* Begin the explicit expected-result failure branch. */
        status = FVM_INVALID_ARGUMENT; /* Supply a visible non-success diagnostic for unexpected
                                          output. */
        goto cleanup;                  /* Release interpreter resources before returning failure. */
    } /* End verification failure handling. */
    printf("Filtered ARGB: %08lx\n",
           (unsigned long)argb); /* Display the successfully verified pixel. */
    exit_code = 0;               /* Select successful process completion only after verification. */

cleanup:                  /* Join all success/error paths at one ownership cleanup point. */
    if (status != FVM_OK) /* Emit a result code for the failed operation. */
        fprintf(stderr, "Color matrix tutorial failed: %d\n",
                (int)status); /* Make failures visible without assertions. */
    fvm_context_destroy(
        context); /* [GC:] Destroy objects, private matrices and all explicit context pins. */
    fvm_runtime_destroy(runtime); /* Release runtime resources after its context is gone. */
    return exit_code;             /* Return zero on verified success or one on failure. */
} /* Finish the backend-independent color matrix tutorial. */
