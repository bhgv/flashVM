/* Independent SOUNDINFO flag combinations, malformed records and copied-event lifetime. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"
#include "zlib.h"

using namespace sound_fixture;


static Bytes control(unsigned flags)
{
    Bytes b;
    word(b, 7);
    b.push_back((unsigned char)flags);
    if (flags & 1)
    {
        integer(b, 0xffffffffUL);
    }
    if (flags & 2)
    {
        integer(b, 0xfffffffeUL);
    }
    if (flags & 4)
    {
        word(b, 65535);
    }
    if (flags & 8)
    {
        b.push_back(2);
        integer(b, 0);
        word(b, 0);
        word(b, 32768);
        integer(b, 44100);
        word(b, 32768);
        word(b, 0);
    }
    return b;
}


static Bytes file(const Bytes &body, unsigned version = 8, bool definition = true)
{
    Bytes defs;
    if (definition)
    {
        const unsigned char data[] = {128};
        tag(defs, 14, sound(7, 12, 1, data, 1));
    }
    tag(defs, 15, body);
    return movie(defs, version);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t n = 99;
    FVM_SWFSoundEvent e;
    e.struct_size = sizeof(e);
    e.sound_id = 99;
    FVM_SWFSoundEnvelope p;
    p.struct_size = sizeof(p);
    p.left_level = 99;
    if (!(fvm_swf_sound_event_count(c, &n) == FVM_UNSUPPORTED && n == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_at(c, 0, &e) == FVM_UNSUPPORTED && e.sound_id == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 1; version <= 8; ++version)
    {
        for (unsigned flags = 0; flags < 64; ++flags)
        {
            Bytes swf = file(control(flags), version);
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_event_count(c, &n) == FVM_OK && n == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_event_at(c, 0, &e) == FVM_OK && e.flags == flags && e.frame == 1 && e.sound_id == 7))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(e.in_point == ((flags & 1) ? 0xffffffffUL : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(e.out_point == ((flags & 2) ? 0xfffffffeUL : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(e.loop_count == ((flags & 4) ? 65535 : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(e.envelope_count == ((flags & 8) ? 2U : 0U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_event_at(c, 1, &e) == FVM_NOT_FOUND && e.sound_id == 7))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (flags & 8)
            {
                if (!(fvm_swf_sound_event_envelope(c, 0, 0, &p) == FVM_OK && p.position_44 == 0 && p.left_level == 0 &&
                      p.right_level == 32768))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_swf_sound_event_envelope(c, 0, 1, &p) == FVM_OK && p.position_44 == 44100 &&
                      p.left_level == 32768 && p.right_level == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_swf_sound_event_envelope(c, 0, 2, &p) == FVM_NOT_FOUND))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    Bytes all = control(63);
    for (size_t cut = 0; cut < all.size(); ++cut)
    {
        Bytes short_body = all;
        short_body.resize(cut);
        Bytes swf = file(short_body);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        n = 99;
        e.sound_id = 99;
        p.left_level = 99;
        if (!(fvm_swf_sound_event_count(c, &n) == FVM_INVALID_ARGUMENT && n == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_at(c, 0, &e) == FVM_INVALID_ARGUMENT && e.sound_id == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_envelope(c, 0, 0, &p) == FVM_INVALID_ARGUMENT && p.left_level == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned bad = 0; bad < 5; ++bad)
    {
        Bytes body = control(8);
        if (bad == 0)
        {
            body[2] |= 128;
        }
        if (bad == 1)
        {
            body[0] = 0;
        }
        if (bad == 2)
        {
            body.push_back(0);
        }
        if (bad == 3)
        {
            body[8] = 1;
            body[9] = 128;
        }
        if (bad == 4)
        {
            body[10] = 255;
            body[11] = 255;
        }
        Bytes swf = file(body);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_count(c, &n) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Largest envelope uses an extended tag header and preserves unsorted raw positions. */
    {
        Bytes body;
        word(body, 7);
        body.push_back(8);
        body.push_back(255);
        for (unsigned i = 0; i < 255; ++i)
        {
            integer(body, 254 - i);
            word(body, i);
            word(body, 32768 - i);
        }
        Bytes swf = file(body);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_at(c, 0, &e) == FVM_OK && e.envelope_count == 255))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_envelope(c, 0, 254, &p) == FVM_OK && p.position_44 == 0 && p.left_level == 254))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_envelope(c, 0, 255, &p) == FVM_NOT_FOUND))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Duplicate selected resource IDs are not silently resolved. */
    {
        Bytes defs;
        const unsigned char data[] = {128};
        tag(defs, 14, sound(7, 12, 1, data, 1));
        tag(defs, 14, sound(7, 12, 1, data, 1));
        tag(defs, 15, control(0));
        Bytes swf = movie(defs);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_count(c, &n) == FVM_OK && n == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_event_at(c, 0, &e) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Forward references resolve; missing IDs remain countable but cannot be selected. */
    Bytes swf = file(control(0), 8, false);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_count(c, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_at(c, 0, &e) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes defs;
    tag(defs, 15, control(0));
    const unsigned char data[] = {128};
    tag(defs, 14, sound(7, 12, 1, data, 1));
    Bytes empty;
    tag(defs, 1, empty);
    tag(defs, 15, control(32));
    swf = movie(defs);
    swf[12] = 2;
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_count(c, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_at(c, 1, &e) == FVM_OK && e.frame == 2 && e.flags == 32))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundEvent copy = e;
    /* CWS canonical ownership and copied records survive subsequent reload. */
    Bytes compressed;
    compressed.resize(8 + compressBound((uLong)(swf.size() - 8)));
    std::memcpy(compressed.begin(), swf.begin(), 8);
    compressed[0] = 'C';
    uLongf size = compressed.size() - 8;
    if (!(compress2(compressed.begin() + 8, &size, swf.begin() + 8, (uLong)(swf.size() - 8), Z_BEST_COMPRESSION) ==
          Z_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    compressed.resize(8 + size);
    if (!(fvm_load_memory(c, compressed.begin(), compressed.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    compressed.clear();
    if (!(fvm_swf_sound_event_at(c, 1, &e) == FVM_OK && e.frame == copy.frame))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    swf[12] = 1;
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_count(c, &n) == FVM_INVALID_ARGUMENT && copy.frame == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_count(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.struct_size = 0;
    if (!(fvm_swf_sound_event_at(c, 0, &e) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    p.struct_size = 0;
    if (!(fvm_swf_sound_event_envelope(c, 0, 0, &p) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
