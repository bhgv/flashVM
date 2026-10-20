/* CLI/timing regression tests do not initialize SDL or open any file. */
#include "../common/options.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    const char *files[16], *error;
    FVM_DesktopOptions options;
    double rate = 99;
    char *valid[] = {"player",  "--no-jit", "--quality", "best",
                     "--speed", "200%",     "a.swf",     "b.swf"};
    char *fps[] = {"player", "--fps", "30", "--frames", "2", "movie.swf"};
    char *mute[] = {"player", "--mute", "movie.swf"};
    char *null_program[] = {0, "--help"};
    char *null_file[] = {"player", 0};
    char *null_operand[] = {"player", "--fps", 0, "a.swf"};
    char *null_after_help[] = {"player", "--help", 0};
    char *version[] = {"player", "--version"};
    char *version_file[] = {"player", "--", "--version"};
    char *help[] = {"player", "--help"};
    char *dash[] = {"player", "--", "--help"};
    char *conflict[] = {"player", "--jit", "--no-jit", "a.swf"};
    char *both[] = {"player", "--fps", "30", "--speed", "100", "a.swf"};
    char *bad[] = {"player", "--fps", "nan", "a.swf"};
    char *quality[] = {"player", "--quality", "fake", "a.swf"};
    const char *numbers[] = {"0", "-1", "inf", "nan", "1x", "1e999", "1001", " 2", "\n2", "0x1p2"};
    size_t i;
    memset(&options, 0, sizeof(options));
    options.frames = 73;
    assert(!fvm_desktop_arguments_valid(-1, valid));
    assert(!fvm_desktop_arguments_valid(0, valid));
    assert(!fvm_desktop_arguments_valid(1, 0));
    assert(fvm_desktop_parse(2, null_program, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR);
    assert(fvm_desktop_parse(2, null_file, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR);
    assert(fvm_desktop_parse(4, null_operand, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR);
    assert(fvm_desktop_parse(3, null_after_help, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR);
    assert(options.frames == 73 && error);
    assert(fvm_desktop_parse(8, valid, files, &options, &error) == FVM_DESKTOP_PARSE_OK);
    assert(!error && options.mode == FVM_EXECUTION_INTERPRETER &&
           options.quality == FVM_STAGE_QUALITY_BEST);
    assert(options.file_count == 2 && !strcmp(files[1], "b.swf"));
    assert(fvm_desktop_rate(&options, 12, &rate) && rate == 24);
    options.percent = 0.1;
    assert(fvm_desktop_rate(&options, 12, &rate) && rate > 0 && rate < 0.1);
    options.percent = 200;
    rate = 24;
    assert(!fvm_desktop_rate(&options, 0, &rate) && rate == 24);
    assert(fvm_desktop_parse(6, fps, files, &options, &error) == FVM_DESKTOP_PARSE_OK);
    assert(options.frames == 2 && fvm_desktop_rate(&options, 0, &rate) && rate == 30);
    assert(fvm_desktop_parse(3, mute, files, &options, &error) == FVM_DESKTOP_PARSE_OK && options.mute);
    assert(fvm_desktop_parse(2, version, files, &options, &error) == FVM_DESKTOP_PARSE_VERSION);
    assert(fvm_desktop_parse(3, version_file, files, &options, &error) == FVM_DESKTOP_PARSE_OK &&
           !strcmp(files[0], "--version"));
    assert(fvm_desktop_parse(2, help, files, &options, &error) == FVM_DESKTOP_PARSE_HELP);
    assert(fvm_desktop_parse(3, dash, files, &options, &error) == FVM_DESKTOP_PARSE_OK &&
           !strcmp(files[0], "--help"));
    assert(fvm_desktop_parse(4, conflict, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR &&
           error);
    assert(fvm_desktop_parse(6, both, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR && error);
    assert(fvm_desktop_parse(4, quality, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR);
    for (i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i)
    {
        bad[2] = (char *)numbers[i];
        assert(fvm_desktop_parse(4, bad, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR &&
               error);
    }
    return 0;
}
