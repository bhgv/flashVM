/* Independent raw-byte oracle for checked C89 visible-row copies and untouched error outputs. */
#include "../common/frame_internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "frame line %d: %s\n", __LINE__, #x);                                                      \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


/* An ordinary function pointer suffices to validate the separate observer policy. */
static FVM_Result observe(const FVM_DesktopFrame *frame, void *data)
{
    (void)frame;
    (void)data;
    return FVM_OK;
}


/* Require every malformed geometry to leave the full caller allocation unchanged. */
static int rejects(FVM_DesktopFrame *frame, size_t capacity, size_t stride)
{
    unsigned char output[32], before[32];
    memset(output, 0xa7, sizeof(output));
    memcpy(before, output, sizeof(output));
    return fvm_desktop_frame_copy(frame, output, capacity, stride) == FVM_INVALID_ARGUMENT &&
           !memcmp(output, before, sizeof(output));
}


int main(void)
{
    static const unsigned char input[] = {1, 2, 3, 4, 5, 6, 7, 8, 99, 98, 97, 96, 9, 10, 11, 12, 13, 14, 15, 16};
    static const unsigned char alias_input[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                                                13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
    static const unsigned char expected[] = {1,    2,    3,    4,    5, 6,  7,  8,  0xa7, 0xa7, 0xa7, 0xa7,
                                             0xa7, 0xa7, 0xa7, 0xa7, 9, 10, 11, 12, 13,   14,   15,   16};
    FVM_DesktopFrameObserver observer;
    FVM_DesktopFrame frame, valid, alias;
    unsigned char output[32];
    CHECK(!fvm_desktop_frame_observer_init(0));
    CHECK(fvm_desktop_frame_observer_init(&observer));
    CHECK(observer.struct_size == sizeof(observer) && !observer.notify && !observer.userdata);
    CHECK(!fvm_desktop_frame_observer_valid(&observer) && !fvm_desktop_frame_observer_valid(0));
    observer.notify = observe;
    CHECK(fvm_desktop_frame_observer_valid(&observer));
    --observer.struct_size;
    CHECK(!fvm_desktop_frame_observer_valid(&observer));
    memset(&frame, 0, sizeof(frame));
    frame.struct_size = sizeof(frame);
    frame.format = FVM_DESKTOP_FRAME_RGBA8;
    frame.width = frame.height = 2;
    frame.stride = 12;
    frame.byte_count = sizeof(input);
    frame.pixels = input;
    memset(output, 0xa7, sizeof(output));
    CHECK(fvm_desktop_frame_copy(&frame, output, sizeof(output), 16) == FVM_OK);
    CHECK(!memcmp(output, expected, sizeof(expected)) && output[31] == 0xa7);
    valid = frame;
    CHECK(rejects(0, sizeof(output), 16));
    --frame.struct_size;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.format = 999;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.pixels = 0;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.width = 0;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.height = 0;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.width = FVM_DESKTOP_FRAME_MAX_DIMENSION + 1;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.height = FVM_DESKTOP_FRAME_MAX_DIMENSION + 1;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.stride = 7;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.byte_count = sizeof(input) - 1;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    frame.stride = (size_t)-1;
    CHECK(rejects(&frame, sizeof(output), 16));
    frame = valid;
    CHECK(rejects(&frame, 23, 16));
    CHECK(rejects(&frame, sizeof(output), 7));
    CHECK(rejects(&frame, (size_t)-1, (size_t)-1));
    CHECK(fvm_desktop_frame_copy(&frame, 0, sizeof(output), 16) == FVM_INVALID_ARGUMENT);
    alias = valid;
    alias.height = 3;
    alias.stride = 8;
    alias.byte_count = sizeof(alias_input);
    alias.pixels = alias_input;
    CHECK(fvm_desktop_frame_copy(&alias, &alias, sizeof(alias), 8) == FVM_OK);
    CHECK(!memcmp((unsigned char *)&alias, alias_input, sizeof(alias_input)));
    frame.height = 1;
    frame.stride = (size_t)-1;
    frame.byte_count = 8;
    CHECK(fvm_desktop_frame_copy(&frame, output, 8, (size_t)-1) == FVM_OK);
    CHECK(!memcmp(output, input, 8));
    puts("PASS raw RGBA row oracle, padding preservation, extent overflow and atomic rejection");
    return 0;
}
