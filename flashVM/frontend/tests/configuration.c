/* Direct C89 configuration validation is deterministic and has no SDL/VM dependency. */
#include "../common/options.h"
#include <assert.h>
#include <float.h>
#include <string.h>

/* Restore a valid borrowed one-file playlist before each independent mutation. */
static void defaults(FVM_DesktopOptions *options, const char **files)
{
    assert(fvm_desktop_options_init(options));
    options->files = files;
    options->file_count = 1;
}

/* Exercise every validation family and preservation of the input structure. */
int main(void)
{
    const char *files[] = {"not-opened.swf", 0};
    const char *error = "sentinel";
    FVM_DesktopOptions options;
    unsigned char before[sizeof(FVM_DesktopOptions)];
    assert(!fvm_desktop_options_init(0));
    assert(!fvm_desktop_options_validate(0, &error) && error);
    defaults(&options, files);
    memcpy(before, &options, sizeof(options));
    assert(fvm_desktop_options_validate(&options, &error) && !error);
    assert(memcmp(&options, before, sizeof(options)) == 0);
    assert(fvm_desktop_options_validate(&options, 0));
    options.mode = (FVM_ExecutionMode)-1;
    assert(!fvm_desktop_options_validate(&options, &error) && error);
    defaults(&options, files);
    options.quality = (FVM_StageQuality)-1;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.quality = (FVM_StageQuality)(FVM_STAGE_QUALITY_BEST + 1);
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.fps = -1;
    assert(!fvm_desktop_options_validate(&options, 0));
    options.fps = DBL_MAX;
    assert(!fvm_desktop_options_validate(&options, 0));
    options.fps = FVM_DESKTOP_MIN_RATE / 2;
    assert(!fvm_desktop_options_validate(&options, 0));
    options.fps = FVM_DESKTOP_MAX_FPS;
    assert(fvm_desktop_options_validate(&options, 0));
    options.percent = FVM_DESKTOP_PERCENT_UNIT / 2;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.percent = 0;
    assert(!fvm_desktop_options_validate(&options, 0));
    options.percent = DBL_MAX;
    assert(!fvm_desktop_options_validate(&options, 0));
    options.percent = FVM_DESKTOP_MAX_PERCENT;
    assert(fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.once = 2;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.mute = -1;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.stats = 2;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.once = 1;
    options.frames = 1;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.files = 0;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.file_count = 0;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.file_count = (size_t)-1;
    assert(!fvm_desktop_options_validate(&options, 0));
    defaults(&options, files);
    options.file_count = 2;
    assert(!fvm_desktop_options_validate(&options, 0));
    files[1] = "";
    assert(!fvm_desktop_options_validate(&options, 0));
    files[1] = "second.swf";
    assert(fvm_desktop_options_validate(&options, &error) && !error);
    return 0;
}
