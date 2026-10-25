/* Direct C89 configuration validation is deterministic and has no SDL/VM dependency. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/options.h"
#include <float.h>
#include <string.h>


/* Restore a valid borrowed one-file playlist before each independent mutation. */
static void defaults(FVM_DesktopOptions *options, const char **files)
{
    if (!(fvm_desktop_options_init(options)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    if (!(!fvm_desktop_options_init(0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_desktop_options_validate(0, &error) && error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    memcpy(before, &options, sizeof(options));
    if (!(fvm_desktop_options_validate(&options, &error) && !error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(memcmp(&options, before, sizeof(options)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.max_loads = 0;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.max_loads = FVM_DESKTOP_MAX_LOADS + 1;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.mode = (FVM_ExecutionMode)-1;
    if (!(!fvm_desktop_options_validate(&options, &error) && error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.quality = (FVM_StageQuality)-1;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.quality = (FVM_StageQuality)(FVM_STAGE_QUALITY_BEST + 1);
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.fps = -1;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.fps = DBL_MAX;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.fps = FVM_DESKTOP_MIN_RATE / 2;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.fps = FVM_DESKTOP_MAX_FPS;
    if (!(fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.percent = FVM_DESKTOP_PERCENT_UNIT / 2;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.percent = 0;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.percent = DBL_MAX;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.percent = FVM_DESKTOP_MAX_PERCENT;
    if (!(fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.once = 2;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.mute = -1;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.stats = 2;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.once = 1;
    options.frames = 1;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.files = 0;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.file_count = 0;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.file_count = (size_t)-1;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    defaults(&options, files);
    options.file_count = 2;
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    files[1] = "";
    if (!(!fvm_desktop_options_validate(&options, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    files[1] = "second.swf";
    if (!(fvm_desktop_options_validate(&options, &error) && !error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
