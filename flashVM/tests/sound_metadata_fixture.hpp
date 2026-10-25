/* Independent tag geometry for six allocation-free sound metadata query routes. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "../src/core/execution_work_scope.hpp"
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"
#include <limits>

using namespace sound_fixture;


static Bytes metadata_movie(unsigned version = 8)
{
    Bytes tags, body;
    const unsigned char pcm[] = {128};
    tag(tags, 14, sound(7, 12, 1, pcm, 1));
    word(body, 7);
    body.push_back(0);
    tag(tags, 15, body);
    body.clear();
    word(body, 7);
    body.push_back(8);
    body.push_back(2);
    integer(body, 0);
    word(body, 0);
    word(body, 32768);
    integer(body, 44100);
    word(body, 32768);
    word(body, 0);
    tag(tags, 15, body);
    return movie(tags, version);
}


/* Five root tags, one definition, two event headers and two envelope points:
 * dictionary pass=6, event pass=9; ordinal definition queries need two passes. */
static unsigned long metadata_units(unsigned route)
{
    const unsigned long costs[] = {6, 6, 12, 9, 15, 15};
    if (!(route < 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return costs[route];
}


static FVM_Result metadata_query(FVM_Context *c, unsigned route, bool unchanged)
{
    size_t count = 12345;
    FVM_SWFSoundInfo info;
    FVM_SWFSoundEvent event;
    FVM_SWFSoundEnvelope point;
    unsigned char old_info[sizeof(info)], old_event[sizeof(event)], old_point[sizeof(point)];
    std::memset(&info, 0xa5, sizeof(info));
    info.struct_size = sizeof(info);
    std::memcpy(old_info, &info, sizeof(info));
    std::memset(&event, 0xa5, sizeof(event));
    event.struct_size = sizeof(event);
    std::memcpy(old_event, &event, sizeof(event));
    std::memset(&point, 0xa5, sizeof(point));
    point.struct_size = sizeof(point);
    std::memcpy(old_point, &point, sizeof(point));
    FVM_Result result = FVM_ERROR;
    if (route == 0)
    {
        result = fvm_swf_sound_count(c, &count);
    }
    if (route == 1)
    {
        result = fvm_swf_sound_info(c, 7, &info);
    }
    if (route == 2)
    {
        result = fvm_swf_sound_at(c, 0, &info);
    }
    if (route == 3)
    {
        result = fvm_swf_sound_event_count(c, &count);
    }
    if (route == 4)
    {
        result = fvm_swf_sound_event_at(c, 0, &event);
    }
    if (route == 5)
    {
        result = fvm_swf_sound_event_envelope(c, 1, 1, &point);
    }
    if (unchanged)
    {
        if (!(count == 12345 && !std::memcmp(&info, old_info, sizeof(info)) &&
              !std::memcmp(&event, old_event, sizeof(event)) && !std::memcmp(&point, old_point, sizeof(point))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (result == FVM_OK)
    {
        if (route == 0)
        {
            if (!(count == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (route == 3)
        {
            if (!(count == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (route == 1 || route == 2)
        {
            if (!(info.sound_id == 7 && info.sample_count == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (route == 4)
        {
            if (!(event.sound_id == 7 && event.frame == 1 && !event.envelope_count))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (route == 5)
        {
            if (!(point.position_44 == 44100 && point.left_level == 32768 && !point.right_level))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    return result;
}
