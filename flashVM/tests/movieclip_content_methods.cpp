/* Versioned ActionScript methods report completed SWF bytes, independent of compressed transport.
 */
#define FVM_AVM1_TARGET_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_target_scope.cpp"
#include "flashvm/serialization.h"
#include "flashvm/execution_mode.h"
#include "flashvm/jit.h"
#include "../src/standard/movieclip_content_methods.hpp"
#include "zlib.h"

static unsigned conversions;


/* Arguments to query methods are ignored without converting them. */
static FVM_Result forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    ++conversions;
    return FVM_ERROR;
}


/* Check a scalar returned through the real public method lookup path. */
static void query(FVM_Context *c, FVM_Object *clip, const char *name, double expected)
{
    FVM_Value out;
    if (!(fvm_object_call(c, clip, name, 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_NUMBER && out.number == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    const unsigned char movie[] = {70, 87, 83, 6, 25, 0, 0, 0, 128, 0, 0, 20, 0, 0, 0, 20, 0, 0, 24, 1, 0, 64, 0, 0, 0};
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned profile = 4; profile <= 8; ++profile)
    {
        for (unsigned mode = 0; mode < 2; ++mode)
        {
            FVM_Context *c = 0;
            FVM_Object *root = 0;
            unsigned char ambient[sizeof(movie)];
            std::memcpy(ambient, movie, sizeof(movie));
            ambient[3] = (unsigned char)profile;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, ambient, sizeof(ambient)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *clip = child(c, root, "clip", 1);
            if (!(fvm_movieclip_replace_swf(c, clip, movie, sizeof(movie), "source.swf", 10) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value out, cv;
            fvm_value_undefined(&cv);
            cv.type = FVM_VALUE_OBJECT;
            cv.object = clip;
            if (!(fvm_object_get(c, clip, "getBytesLoaded", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (profile < 5)
            {
                if (!(out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
                continue;
            }
            query(c, clip, "getBytesLoaded", sizeof(movie));
            query(c, clip, "getBytesTotal", sizeof(movie));
            if (!(fvm_object_call(c, root, "getBytesTotal", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, clip, "getSWFVersion", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (profile < 7)
            {
                if (!(out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                query(c, clip, "getSWFVersion", 6);
                query(c, root, "getSWFVersion", profile);
            }
            unsigned char compressed[128];
            uLongf length = sizeof(compressed) - 8;
            std::memcpy(compressed, movie, 8);
            compressed[0] = 'C';
            if (!(compress2(compressed + 8, &length, movie + 8, sizeof(movie) - 8, Z_BEST_COMPRESSION) == Z_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_replace_swf(c, clip, compressed, (size_t)length + 8, "source.swf", 10) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            query(c, clip, "getBytesTotal", sizeof(movie));
            /* Actual CallMethod actions must enter emitted code when a native backend is available.
             */
            if (!(fvm_set_global(c, "meterClip", &cv) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes actions;
            text(actions, "measured");
            integer(actions, 0);
            global(actions, "meterClip");
            text(actions, "getBytesLoaded");
            actions.push_back(0x52);
            actions.push_back(0x1d);
            if (!(execute(c, root, actions) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, root, "measured", sizeof(movie));
            FVM_ExecutionStats stats;
            stats.struct_size = sizeof(stats);
            if (!(fvm_execution_get_stats(c, &stats) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (mode && fvm_jit_available())
            {
                if (!(stats.native_entries))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(stats.interpreter_entries))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            FVM_Object *argument = 0;
            if (!(fvm_object_create(c, &argument) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_method_rooted(c, argument, "valueOf", forbidden, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_method_rooted(c, argument, "toString", forbidden, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value input;
            fvm_value_undefined(&input);
            input.type = FVM_VALUE_OBJECT;
            input.object = argument;
            conversions = 0;
            if (!(fvm_object_call(c, clip, "getBytesTotal", 1, &input, &out) == FVM_OK && out.number == sizeof(movie) &&
                  !conversions))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Method identities and source counts survive graph transport to a new context. */
            FVM_Context *other = 0;
            if (!(fvm_context_create(rt, &other) == FVM_OK && fvm_standard_install(other) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char *wire = 0;
            size_t wire_size = 0;
            if (!(fvm_value_serialize(c, &cv, &wire, &wire_size) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_value_deserialize(other, wire, wire_size, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_serialized_free(wire);
            FVM_Object *copy = out.object;
            if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            query(other, copy, "getBytesLoaded", sizeof(movie));
            query(other, copy, "getSWFVersion", 6);
            if (!(fvm_movieclip_unload(other, copy) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            query(other, copy, "getBytesLoaded", 0);
            query(other, copy, "getBytesTotal", 0);
            query(other, copy, "getSWFVersion", -1);
            query(c, clip, "getBytesTotal", sizeof(movie));
            fvm_context_destroy(other);
            FVM_NativeFunction callbacks[] = {
                fvm_movieclip_bytes_loaded, fvm_movieclip_bytes_total, fvm_movieclip_swf_version};
            for (unsigned i = 0; i < 3; ++i)
            {
                unsigned long id = fvm_movieclip_callback_id(callbacks[i]);
                if (!(id && fvm_movieclip_callback_from_id(id) == callbacks[i]))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_movieclip_unload(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            query(c, clip, "getBytesLoaded", 0);
            if (profile >= 7)
            {
                query(c, clip, "getSWFVersion", -1);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
