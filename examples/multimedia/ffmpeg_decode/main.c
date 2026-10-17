/* Tutorial: use the real FFmpeg decoder without hidden playback or camera access. */
#include "flashvm/multimedia.h" /* Import the replaceable decoder and capture ABI. */
#include <stdio.h> /* Report decoded metadata and ordinary errors. */
#include <string.h> /* Recognize explicit source-kind arguments. */
/* Decode an explicit audio/video file, or one frame from an explicitly named camera. */
int main(int argc, char **argv) /* No arguments prints usage without opening any resource. */
{ /* Begin explicit host-driven decoding. */
    FVM_MediaBackend backend; /* Receive a real static FFmpeg backend table. */
    FVM_MediaSession *session = 0; /* Own one native session until common close. */
    FVM_MediaFrame frame; /* Borrow one converted PCM/RGBA frame at a time. */
    FVM_Result result; /* Track decoder availability, I/O failures and EOF. */
    unsigned long frames = 0; /* Count returned buffers without retaining their pointers. */
    int camera = /* Require both device and explicit format for camera access. */
        argc == 4 && strcmp(argv[1], "camera") == 0; /* Require both device and explicit format for camera access. */
    if ((argc != 3 && !camera) || /* Validate argument shape before touching external resources. */
        (argc == 3 && strcmp(argv[1], "audio") && /* Validate argument shape before touching external resources. */
         strcmp(argv[1], "video"))) /* Validate argument shape before touching external resources. */
    { /* Show the host-controlled source selection. */
        puts("Usage: ffmpeg_decode audio|video filename OR camera device format"); /* Name both file
                                                                                      and explicit
                                                                                      camera forms.
                                                                                    */
        return argc == 1 ? 0 : 1; /* Keep automatic no-argument smoke execution harmless. */
    } /* Finish usage handling. */
    result = /* Initialize actual library callbacks or return UNSUPPORTED in NO_MEDIA. */
        fvm_media_ffmpeg_backend(&backend); /* Initialize actual library callbacks or return UNSUPPORTED in NO_MEDIA. */
    if (result != FVM_OK) /* Explain the optional dependency instead of pretending to decode. */
    { /* Report unavailable backend construction. */
        fprintf(stderr, "FFmpeg unavailable (%d): use MEDIA and FVM_FFMPEG_ROOT.\n", /* Point to the required build configuration. */
                (int)result); /* Point to the required build configuration. */
        return 1; /* Do not continue without real native callbacks. */
    } /* Finish unavailable-backend handling. */
    if (camera) /* Open only the explicitly requested native camera device. */
        result = fvm_camera_open(&backend, argv[2], argv[3], /* Open only the explicitly requested native camera device. */
                                 &session); /* Open only the explicitly requested native camera device. */
    else if (strcmp(argv[1], "audio") == 0) /* Select one audio stream from the file. */
        result = fvm_audio_open(&backend, argv[2], &session); /* Select one audio stream from the file. */
    else /* Select one video stream and skip unrelated audio packets. */
        result = /* Select one video stream and skip unrelated audio packets. */
            fvm_video_open(&backend, argv[2], &session); /* Select one video stream and skip unrelated audio packets. */
    if (result != FVM_OK) /* Report unsupported codecs/devices and open failures normally. */
    { /* No native session survives a failed open. */
        fprintf(stderr, "Open failed: %d\n", (int)result); /* Preserve the public error category. */
        return 1; /* Finish without an invalid cleanup call. */
    } /* Finish open error handling. */
    while ((result = fvm_media_read(session, &frame)) == FVM_OK) /* Decode real buffers synchronously. */
    { /* Consume metadata before the next read invalidates its borrowed bytes. */
        ++frames; /* Count only successful decoded buffers. */
        if (frames == 1) /* Show actual first-buffer properties. */
            printf("%s: kind=%d, bytes=%lu, size=%ux%u, audio=%uHz/%uch\n", backend.os_name, frame.kind, /* Show actual first-buffer properties. */
                   (unsigned long)frame.size, frame.width, frame.height, frame.sample_rate, /* Show actual first-buffer properties. */
                   frame.channels); /* Show actual first-buffer properties. */
        if (camera) /* Bound the camera example to one frame, then release the real device. */
            break; /* Bound the camera example to one frame, then release the real device. */
    } /* Finish decoding. */
    fvm_media_close(session); /* Release native demuxer, decoder, buffers and any opened capture device. */
    if (result != FVM_NOT_FOUND && /* Distinguish file EOF or one camera frame from a hard failure. */
        !(camera && result == FVM_OK)) /* Distinguish file EOF or one camera frame from a hard failure. */
    { /* Report read failure after deterministic resource cleanup. */
        fprintf(stderr, "Decode failed: %d\n", (int)result); /* Preserve actual native decoder errors. */
        return 1; /* Do not claim a complete successful decode. */
    } /* Finish read error reporting. */
    printf("Decoded %lu buffers; playback/presentation is controlled by the host.\n", /* State the actual scope of this example. */
           frames); /* State the actual scope of this example. */
    return 0; /* Finish the native decoding tutorial. */
} /* Finish main. */
