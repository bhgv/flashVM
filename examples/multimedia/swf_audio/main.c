/* Tutorial: enumerate loaded SWF DefineSound resources and explicitly export one PCM sound through a channel. */
#include "flashvm/swf_sound.h" /* Declare root sound inspection, bounded PCM decoding and explicit submission. */
#include "flashvm/avm1.h" /* Stop the registered host output through real AVM1 StopSounds. */
#include <stdio.h> /* Read the explicitly supplied SWF and implement real PCM file output. */
#include <stdlib.h> /* Allocate bounded input storage and parse the requested numeric sound ID. */
#include <string.h> /* Initialize native host state before partial setup. */
#include <errno.h> /* Reject malformed or overflowing numeric sound IDs. */
#define TUTORIAL_SWF_INPUT_LIMIT (64UL * 1024UL * 1024UL) /* Bound the explicit input-file allocation. */
#define TUTORIAL_SWF_SOUND_ID_MAX 65535UL /* Sound IDs are nonzero unsigned sixteen-bit dictionary keys. */
#define TUTORIAL_SWF_PCM_CHUNK 256U /* Submit small explicit sample-frame ranges instead of allocating an entire song. */
typedef struct PcmFile /* Keep real native output ownership outside the VM collector. */
{ /* Begin host-owned file state. */
    const char *path; /* Borrow argv storage until all context callbacks finish. */
    FILE *file; /* Own the explicitly opened output file until stop or cleanup. */
} PcmFile; /* Finish output ownership state. */
/* Consume a synchronously borrowed decoded PCM frame using the real host C library. */
static FVM_Result write_pcm(void *raw, const FVM_MediaFrame *frame) /* Never retain the temporary frame view. */
{ /* Begin real file output. */
    PcmFile *host = (PcmFile *)raw; /* Recover host state which outlives the context. */
    if (!host->file) /* Open output only after a supported nonempty sound reaches this callback. */
        host->file = fopen(host->path, "wb"); /* Create or replace the explicitly named raw PCM file. */
    if (!host->file) /* Return a native open error without acknowledging submission. */
        return FVM_ERROR; /* No channel activation is published. */
    return fwrite(frame->data, 1, frame->size, host->file) == frame->size ? FVM_OK : FVM_ERROR; /* Consume all borrowed bytes before returning. */
} /* Finish real PCM output. */
/* Stop this observable file sink by closing its native resource. */
static FVM_Result stop_pcm(void *raw) /* A real sound-card bridge would discard its queued device samples here. */
{ /* Begin host stopping. */
    PcmFile *host = (PcmFile *)raw; /* Recover the same borrowed output owner. */
    int result = 0; /* Preserve actual fclose failure. */
    if (host->file) /* Avoid closing a resource which was never opened. */
    { /* Close the one acknowledged output resource. */
        result = fclose(host->file); /* Flush and close through the real C library. */
        host->file = 0; /* fclose consumes its FILE even when reporting failure. */
    } /* Finish native output closure. */
    return result == 0 ? FVM_OK : FVM_ERROR; /* Propagate actual stop status. */
} /* Finish host stopping. */
/* Report ordinary API failures and route the caller to explicit cleanup. */
static int checked(FVM_Result result, const char *operation) /* Borrow the operation label only while printing. */
{ /* Begin normal error reporting. */
    if (result == FVM_OK) /* Require acknowledged success. */
        return 1; /* Continue the tutorial. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Keep unsupported codec/width and malformed data visible. */
    return 0; /* Preserve centralized cleanup. */
} /* Finish error reporting. */
/* Inspect copied root StartSound controls before exporting any PCM. */
static int show_events(FVM_Context *context) /* Keep timeline inspection independent of playback policy. */
{ /* Begin allocation-free event enumeration. */
    size_t count, index, point; /* Use independent zero-based event and envelope ordinals. */
    FVM_SWFSoundEvent event; /* Receive copied control fields without retaining source pointers. */
    FVM_SWFSoundEnvelope envelope; /* Receive one copied gain/time point at a time. */
    if (!checked(fvm_swf_sound_event_count(context, &count), "sound_event_count")) /* Validate every root control and frame boundary. */
        return 0; /* Keep malformed timelines visible before output creation. */
    for (index = 0; index < count; ++index) /* Preserve wire order, including controls sharing a frame. */
    { /* Inspect one event and resolve its unique DefineSound. */
        event.struct_size = sizeof(event); /* Declare the exact copied-event ABI. */
        if (!checked(fvm_swf_sound_event_at(context, index, &event), "sound_event_at")) /* Reject missing or duplicate selected resources. */
            return 0; /* Do not invent a resource for a malformed event reference. */
        printf("Event %lu: frame %u, ID %u, flags %u, in %lu, last %lu, loops %u, envelope %u\n", (unsigned long)index, event.frame, event.sound_id, event.flags, event.in_point, event.out_point, event.loop_count, event.envelope_count); /* Print raw fields; flags distinguish omitted values from explicit zero. */
        for (point = 0; point < event.envelope_count; ++point) /* Inspect every point without allocating an array. */
        { /* Preserve the envelope's independent 44100 Hz time base. */
            envelope.struct_size = sizeof(envelope); /* Declare the exact copied-envelope ABI. */
            if (!checked(fvm_swf_sound_event_envelope(context, index, point, &envelope), "sound_event_envelope")) /* Re-query current canonical bytes safely. */
                return 0; /* Keep malformed or missing points visible. */
            printf("  Point %lu: position44 %lu, left %u, right %u\n", (unsigned long)point, envelope.position_44, envelope.left_level, envelope.right_level); /* Show gain values in 0..32768 units without applying them. */
        } /* Finish one event's envelope inspection. */
    } /* Finish root event enumeration. */
    return 1; /* Inspection succeeded without playing, stopping or scheduling sound. */
} /* Finish copied StartSound inspection. */
/* Read an explicitly named file, enumerate root sounds and export a caller-selected PCM or ADPCM sound as S16. */
int main(int argc, char **argv) /* Keep the tutorial valid C89 without assertions. */
{ /* Begin input, VM and output ownership. */
    FVM_Config config; /* Initialize before creating the runtime. */
    FVM_Runtime *runtime = 0; /* Track partial runtime acquisition. */
    FVM_Context *context = 0; /* Own canonical loaded SWF bytes and channel registrations. */
    FVM_AudioChannel *channel = 0; /* Borrow the context-owned output registration. */
    FVM_MediaOutput output; /* Supply host callbacks which registration copies. */
    FVM_SWFSoundInfo info; /* Receive nominal metadata and a borrowed SoundData view. */
    PcmFile host; /* Keep native output state alive until context destruction has returned. */
    FILE *input = 0; /* Own only the explicitly requested input file. */
    unsigned char *bytes = 0; /* Own temporary raw input separately from canonical VM storage. */
    long length; /* Read and validate the file length before allocating it. */
    char *end; /* Validate complete numeric ID parsing. */
    unsigned long sound_id, first; /* Track the selected dictionary key and explicit sample offset. */
    size_t sounds, index, frames; /* Count root definitions and bounded sample-frame ranges. */
    const unsigned char stop[] = {0x09, 0x00}; /* Encode StopSounds followed by End. */
    int exit_code = 1; /* Report success only after explicit export and stopping complete. */
    memset(&host, 0, sizeof(host)); /* Make cleanup safe after partial setup. */
    if (argc != 4) /* Require explicit input, sound ID and output before file operations. */
    { /* Explain observable side effects without opening anything. */
        printf("Usage: %s INPUT.swf SOUND_ID OUTPUT.pcm\nExports raw native-endian S16; replaces the named output.\n", argv[0]); /* Describe output representation and replacement. */
        return 0; /* Keep no-argument test runs free of file/device side effects. */
    } /* Finish the explicit input gate. */
    if (!*argv[2] || strspn(argv[2], "0123456789") != strlen(argv[2])) /* Accept digits only, avoiding signed strtoul wraparound and partial text. */
    { /* Reject an invalid decimal resource key before file access. */
        fprintf(stderr, "SOUND_ID must contain decimal digits only.\n"); /* Explain why the explicit key is rejected. */
        goto cleanup; /* No native file or VM exists yet. */
    } /* Finish lexical resource-key validation. */
    errno = 0; /* Detect strtoul overflow rather than silently truncating IDs. */
    sound_id = strtoul(argv[2], &end, 10); /* Parse a decimal root dictionary ID. */
    if (errno || !sound_id || sound_id > TUTORIAL_SWF_SOUND_ID_MAX || *end) /* Reject partial, negative-wrapped and overflowing keys. */
    { /* Explain the invalid caller-selected resource. */
        fprintf(stderr, "SOUND_ID must be 1..65535.\n"); /* Avoid treating arbitrary text as a dictionary key. */
        goto cleanup; /* No output has been opened. */
    } /* Finish resource-key validation. */
    input = fopen(argv[1], "rb"); /* Open the explicitly supplied SWF input through the real C library. */
    if (!input || fseek(input, 0, SEEK_END) != 0) /* Obtain a bounded seekable input length. */
    { /* Keep a failed native input operation visible. */
        fprintf(stderr, "Cannot open or seek the input SWF.\n"); /* Explain the failed host operation. */
        goto cleanup; /* Preserve any acquired FILE for cleanup. */
    } /* Finish native input failure handling. */
    length = ftell(input); /* Inspect the actual host file size. */
    if (length <= 0 || (unsigned long)length > TUTORIAL_SWF_INPUT_LIMIT || fseek(input, 0, SEEK_SET) != 0) /* Reject invalid/oversized input before allocation. */
    { /* Explain why the native input cannot be retained. */
        fprintf(stderr, "Input must be a nonempty seekable SWF of at most 64 MiB.\n"); /* Describe the explicit resource bound. */
        goto cleanup; /* No VM or output exists yet. */
    } /* Finish input-length failure handling. */
    bytes = (unsigned char *)malloc((size_t)length); /* Own one bounded input copy. */
    if (!bytes || fread(bytes, 1, (size_t)length, input) != (size_t)length) /* Require the complete requested file contents. */
    { /* Report native allocation/read failure normally. */
        fprintf(stderr, "Cannot allocate or read the complete input SWF.\n"); /* Keep host I/O failure distinct from unsupported audio. */
        goto cleanup; /* Release native input storage normally. */
    } /* Finish native input-copy failure handling. */
    if (!checked(fvm_config_init(&config), "config_init")) /* Initialize the public embedding ABI. */
        goto cleanup; /* Keep native input cleanup available. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime_create")) /* Acquire the runtime. */
        goto cleanup; /* No context exists on failure. */
    if (!checked(fvm_context_create(runtime, &context), "context_create")) /* Acquire canonical SWF storage ownership. */
        goto cleanup; /* Keep runtime alive until context cleanup. */
    if (!checked(fvm_load_memory(context, bytes, (size_t)length), "load_memory")) /* Decode FWS/CWS and retain canonical bytes in the context. */
        goto cleanup; /* Malformed structure does not open output. */
    free(bytes); /* The loaded context owns an independent canonical copy. */
    bytes = 0; /* Prevent cleanup from freeing the input copy twice. */
    if (!checked(fvm_swf_sound_count(context, &sounds), "sound_count")) /* Validate root sound headers and count occurrences. */
        goto cleanup; /* Keep malformed resource errors visible. */
    for (index = 0; index < sounds; ++index) /* Enumerate definitions in their root file order. */
    { /* Inspect one definition without allocating or opening a device. */
        info.struct_size = sizeof(info); /* Declare the exact view ABI. */
        if (!checked(fvm_swf_sound_at(context, index, &info), "sound_at")) /* Reject ambiguous duplicate selected IDs. */
            goto cleanup; /* Never silently select one duplicate resource. */
        printf("ID %u: format %u, %u Hz, %u channels, %u bits, %lu frames\n", info.sound_id, info.format, info.sample_rate, info.channels, info.bits_per_sample, info.sample_count); /* Show nominal header data, not a compressed-codec probe. */
    } /* Finish definition enumeration. */
    if (!show_events(context)) /* Inspect StartSound/SOUNDINFO without executing its playback controls. */
        goto cleanup; /* Reject malformed event metadata before creating output. */
    info.struct_size = sizeof(info); /* Reuse the exact inspection ABI for selected lookup. */
    if (!checked(fvm_swf_sound_info(context, (unsigned int)sound_id, &info), "sound_info")) /* Resolve the caller's unique root sound. */
        goto cleanup; /* NOT_FOUND does not create output. */
    host.path = argv[3]; /* Borrow the explicitly named output path through cleanup. */
    output.struct_size = sizeof(output); /* Supply the complete host callback ABI. */
    output.userdata = &host; /* Keep file ownership outside the managed collector. */
    output.write = write_pcm; /* Delegate actual output to synchronous fwrite. */
    output.stop = stop_pcm; /* Delegate StopSounds to synchronous fclose. */
    if (!checked(fvm_audio_channel_create(context, &output, &channel), "channel_create")) /* Copy the table into an inactive registration. */
        goto cleanup; /* Creation itself does not open output. */
    first = 0; /* Start at the first declared sample frame. */
    do /* Validate even a zero-frame sound's format once, without activating output. */
    { /* Submit one explicitly bounded sample-frame interval. */
        frames = info.sample_count - first; /* Determine remaining complete frames. */
        if (frames > TUTORIAL_SWF_PCM_CHUNK) /* Bound each temporary native PCM allocation. */
            frames = TUTORIAL_SWF_PCM_CHUNK; /* Let the host choose a small explicit chunk policy. */
        if (!checked(fvm_swf_sound_submit(context, (unsigned int)sound_id, channel, first, frames), "sound_submit")) /* Decode supported PCM/ADPCM to S16 and submit a callback-independent copy. */
            goto cleanup; /* Unsupported codec/width never masquerades as decoded audio. */
        first += (unsigned long)frames; /* Advance by acknowledged sample frames without replay. */
    } while (first < info.sample_count); /* Finish only after the selected source range is exported. */
    if (!checked(fvm_avm1_execute(context, stop, sizeof(stop)), "StopSounds")) /* Stop the registered file sink through the real action. */
        goto cleanup; /* Keep host state alive for terminal context cleanup. */
    printf("Exported %lu sample frames; raw PCM only, no audible playback.\n", first); /* Report observable export rather than a sound-card claim. */
    exit_code = 0; /* All demonstrated operations completed. */
cleanup: /* Keep native userdata valid until the context has finished callbacks. */
    if (context) /* Destroy only a successfully acquired context. */
        fvm_context_destroy(context); /* Release registrations and canonical bytes after terminal stopping. */
    if (host.file) /* A failed write can retain a host resource outside acknowledged channel state. */
        fclose(host.file); /* Release that resource explicitly after context callbacks finish. */
    if (runtime) /* Runtime must outlive its context. */
        fvm_runtime_destroy(runtime); /* Release runtime storage last among VM handles. */
    if (input) /* Release the independently owned input file. */
        fclose(input); /* Never close an absent FILE. */
    free(bytes); /* Release any unadopted temporary input copy; null is safe. */
    return exit_code; /* Return ordinary status without assertions or abort. */
} /* Finish explicit SWF PCM export. */
