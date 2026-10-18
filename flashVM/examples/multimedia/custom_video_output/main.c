/* Tutorial: bridge a borrowed RGBA image to a real host PPM file writer. */
#include "flashvm/multimedia.h" /* Import the independent output callback ABI. */
#include <stdio.h> /* Use real fopen, fprintf and fwrite system-library functions. */
#include <string.h> /* Initialize the public output and frame structures. */
/* Consume the RGBA view synchronously; PPM stores RGB so alpha is intentionally omitted. */
static FVM_Result ppm_present(void *opaque, const FVM_MediaFrame *frame) /* Borrow both the host FILE and frame. */
{ /* Begin the actual host output bridge. */
    FILE *file = (FILE *)opaque; /* Recover the caller-owned output file. */
    unsigned int y; /* Traverse complete rows without assuming a fixed frame height. */
    unsigned int x; /* Traverse pixels without retaining any borrowed view. */
    if (fprintf(file, "P6\n%u %u\n255\n", frame->width, frame->height) < 0) /* Write the real PPM header. */
        return FVM_ERROR; /* Write the real PPM header. */
    for (y = 0; y < frame->height; ++y) /* Respect the producer's validated row stride. */
    { /* Begin one image row. */
        const unsigned char *row = frame->data + y * frame->stride; /* Locate this borrowed RGBA row. */
        for (x = 0; x < frame->width; ++x) /* Convert every pixel to PPM RGB. */
        { /* Begin one synchronous pixel write. */
            if (fwrite(row + x * FVM_MEDIA_RGBA_BYTES_PER_PIXEL, 1, 3, file) != 3) /* Write real initialized RGB bytes and preserve errors. */
                return FVM_ERROR; /* Write real initialized RGB bytes and preserve errors. */
        } /* Finish pixel writing. */
    } /* Finish row writing. */
    return FVM_OK; /* The caller retains file ownership and handles final close errors. */
} /* Finish the custom presenter. */
/* Create a tiny opaque image and present it through the same ABI used by a window backend. */
int main(int argc, char **argv) /* Require an explicit output filename before writing. */
{ /* Begin the standalone output tutorial. */
    const unsigned char pixels[8] = {255, 0,   0, 255, /* Keep a red/green RGBA pair alive throughout presentation. */
                                     0,   255, 0, 255}; /* Keep a red/green RGBA pair alive throughout presentation. */
    FVM_MediaFrame frame; /* Describe a borrowed view of the stack image. */
    FVM_MediaOutput output; /* Connect the host's real file presentation callback. */
    FVM_Result result; /* Track presentation and close errors normally. */
    FILE *file; /* Own the explicit host destination until fclose. */
    if (argc != 2) /* Avoid writing files during the automatic no-argument smoke run. */
    { /* Explain the destination argument. */
        puts("Usage: custom_video_output output.ppm"); /* Describe the created artifact. */
        return argc == 1 ? 0 : 1; /* Treat malformed arguments as an ordinary error. */
    } /* Finish usage handling. */
    file = fopen(argv[1], "wb"); /* Open exactly the host-requested output path, replacing its existing contents. */
    if (!file) /* Report actual host-library failure. */
    { /* Preserve normal error handling without assertions. */
        perror("Output open failed"); /* Include the OS error message. */
        return 1; /* No native handle needs cleanup after failed fopen. */
    } /* Finish open failure handling. */
    memset(&frame, 0, sizeof(frame)); /* Leave unrelated audio/timestamp metadata empty. */
    frame.kind = FVM_MEDIA_VIDEO; /* Select RGBA video presentation. */
    frame.data = pixels; /* Borrow stack data until the synchronous callback returns. */
    frame.size = sizeof(pixels); /* Bound all accessible frame bytes. */
    frame.width = 2; /* Describe the two initialized pixels. */
    frame.height = 1; /* Describe one image row. */
    frame.stride = sizeof(pixels); /* Store the byte distance between complete rows. */
    memset(&output, 0, sizeof(output)); /* Leave optional audio stop unsupported. */
    output.struct_size = sizeof(output); /* Identify the complete output-table ABI. */
    output.userdata = file; /* Borrow the host-owned destination handle. */
    output.write = ppm_present; /* Install the real synchronous output bridge. */
    result = fvm_video_present(&output, &frame); /* Validate bounds before invoking the host callback. */
    if (fclose(file) != 0) /* Finish output and preserve delayed write/flush errors. */
        result = FVM_ERROR; /* Finish output and preserve delayed write/flush errors. */
    if (result != FVM_OK) /* Report incomplete output normally. */
    { /* Produce one useful result diagnostic. */
        fprintf(stderr, "Presentation failed: %d\n", (int)result); /* Describe the actual failure. */
        return 1; /* Let the caller decide whether to remove a partially written file. */
    } /* Finish error reporting. */
    puts("Wrote one 2x1 PPM image through the host output bridge."); /* State the actual
                                                                        demonstrated output. */
    return 0; /* Finish the custom video output tutorial. */
} /* Finish main. */
