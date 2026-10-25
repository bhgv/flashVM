/* CLI/timing regression tests do not initialize SDL or open any file. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/options.h"
#include <string.h>


int main(void)
{
    const char *files[16], *error;
    FVM_DesktopOptions options;
    double rate = 99;
    char *valid[] = {"player", "--no-jit", "--quality", "best", "--speed", "200%", "a.swf", "b.swf"};
    char *fps[] = {"player", "--fps", "30", "--frames", "2", "movie.swf"};
    char *finite_audio[] = {"player", "--finite-audio-tails", "movie.swf"};
    char *finite_audio_file[] = {"player", "--", "--finite-audio-tails"};
    char *finite[] = {"player", "--finite-mixed-tails", "movie.swf"};
    char *finite_file[] = {"player", "--", "--finite-mixed-tails"};
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
    char *loads[] = {"player", "--loads", "3", "movie.swf"};
    if (!(fvm_desktop_parse(4, loads, files, &options, &error) == FVM_DESKTOP_PARSE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(options.max_loads == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    loads[2] = "1025";
    if (!(fvm_desktop_parse(4, loads, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(options.max_loads == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&options, 0, sizeof(options));
    options.frames = 73;
    if (!(!fvm_desktop_arguments_valid(-1, valid)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_desktop_arguments_valid(0, valid)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_desktop_arguments_valid(1, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(2, null_program, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(2, null_file, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(4, null_operand, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(3, null_after_help, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(options.frames == 73 && error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(8, valid, files, &options, &error) == FVM_DESKTOP_PARSE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!error && options.mode == FVM_EXECUTION_INTERPRETER && options.quality == FVM_STAGE_QUALITY_BEST))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(options.file_count == 2 && !strcmp(files[1], "b.swf")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_rate(&options, 12, &rate) && rate == 24))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.percent = 0.1;
    if (!(fvm_desktop_rate(&options, 12, &rate) && rate > 0 && rate < 0.1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.percent = 200;
    rate = 24;
    if (!(!fvm_desktop_rate(&options, 0, &rate) && rate == 24))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(6, fps, files, &options, &error) == FVM_DESKTOP_PARSE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(options.frames == 2 && fvm_desktop_rate(&options, 0, &rate) && rate == 30))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(3, mute, files, &options, &error) == FVM_DESKTOP_PARSE_OK && options.mute))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!options.finite_mixed_tails))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(3, finite, files, &options, &error) == FVM_DESKTOP_PARSE_OK && options.finite_mixed_tails &&
          !options.mute))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.finite_mixed_tails = 2;
    if (!(!fvm_desktop_options_validate(&options, &error) && error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.finite_mixed_tails = 1;
    if (!(fvm_desktop_options_validate(&options, &error)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(3, finite_file, files, &options, &error) == FVM_DESKTOP_PARSE_OK &&
          !options.finite_mixed_tails && !strcmp(files[0], "--finite-mixed-tails")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(3, finite_audio, files, &options, &error) == FVM_DESKTOP_PARSE_OK &&
          options.finite_audio_tails && !options.finite_mixed_tails && !options.mute))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    options.finite_audio_tails = 2;
    if (!(!fvm_desktop_options_validate(&options, &error) && error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(3, finite_audio_file, files, &options, &error) == FVM_DESKTOP_PARSE_OK &&
          !options.finite_audio_tails && !strcmp(files[0], "--finite-audio-tails")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(2, version, files, &options, &error) == FVM_DESKTOP_PARSE_VERSION))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(3, version_file, files, &options, &error) == FVM_DESKTOP_PARSE_OK &&
          !strcmp(files[0], "--version")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(2, help, files, &options, &error) == FVM_DESKTOP_PARSE_HELP))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(3, dash, files, &options, &error) == FVM_DESKTOP_PARSE_OK && !strcmp(files[0], "--help")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(4, conflict, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR && error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(6, both, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR && error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_parse(4, quality, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i)
    {
        bad[2] = (char *)numbers[i];
        if (!(fvm_desktop_parse(4, bad, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR && error))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
