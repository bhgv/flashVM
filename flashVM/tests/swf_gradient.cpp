#define FVM_SHAPE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "swf_shape.cpp"


static inline Bytes gradient_shape(unsigned int kind = 0x10, unsigned int tag_code = 32, size_t *header_offset = 0)
{
    Bytes b;
    u16(b, 9);
    Bits bounds;
    bounds.put(16, 5);
    bounds.put(0, 16);
    bounds.put(320, 16);
    bounds.put(0, 16);
    bounds.put(320, 16);
    append(b, bounds.b);
    b.push_back(1);
    b.push_back((unsigned char)kind);
    Bits matrix;
    matrix.put(1, 1);
    matrix.put(11, 5);
    matrix.put(640, 11);
    matrix.put(640, 11);
    matrix.put(0, 1);
    matrix.put(16, 5);
    matrix.put(160, 16);
    matrix.put(160, 16);
    append(b, matrix.b);
    if (header_offset)
    {
        *header_offset = b.size();
    }
    b.push_back(2);
    b.push_back(0);
    b.push_back(255);
    b.push_back(0);
    b.push_back(0);
    if (tag_code == 32)
    {
        b.push_back(255);
    }
    b.push_back(255);
    b.push_back(0);
    b.push_back(0);
    b.push_back(255);
    if (tag_code == 32)
    {
        b.push_back(255);
    }
    b.push_back(0);
    Bits records;
    records.put(1, 4);
    records.put(0, 4);
    records.put(0, 1);
    records.put(3, 5);
    records.put(16, 5);
    records.put(0, 16);
    records.put(0, 16);
    records.put(1, 1);
    edge(records, 320, 0);
    edge(records, 0, 320);
    edge(records, -320, 0);
    edge(records, 0, -320);
    records.put(0, 6);
    append(b, records.b);
    return b;
}


static inline void gradient_pixels(FVM_Context *c, FVM_Object *root, bool radial)
{
    unsigned char pixels[16 * 16 * 4];
    std::memset(pixels, 0, sizeof(pixels));
    FVM_RasterSurface surface = {pixels, 16, 16, 64};
    FVM_Result r = fvm_movieclip_render(c, root, &surface);
    if (r == FVM_UNSUPPORTED)
    {
        return;
    }
    if (!(r == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char *p = pixels + 4 * (8 * 16 + 8);
    unsigned int expected_red = radial ? 232 : 120, expected_blue = radial ? 23 : 135;
    if (!((unsigned int)p[0] + 1 >= expected_red && p[0] <= expected_red + 1 && p[1] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!((unsigned int)p[2] + 1 >= expected_blue && p[2] <= expected_blue + 1 && p[3] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_GRADIENT_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
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
    if (!(fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned int variant = 0; variant < 6; ++variant)
    {
        unsigned int tag_code = variant / 2 == 0 ? 2 : variant / 2 == 1 ? 22 : 32;
        bool radial = (variant & 1) != 0;
        Bytes payload = gradient_shape(radial ? 0x12 : 0x10, tag_code);
        FVM_SWFShape decoded;
        if (!(fvm_swf_read_shape(payload.begin(), payload.size(), tag_code, decoded) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoded.paints.size() == 1 && decoded.paints[0].stops.size() == 2 &&
              decoded.paints[0].gradient_matrix.x0 == 8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes tags, empty;
        tag(tags, tag_code, payload);
        tag(tags, 26, place(2 | 4, 1, 9, 0, 0));
        tag(tags, 1, empty);
        tag(tags, 0, empty);
        FVM_Object *root = load(c, tags, 1);
        tick(c, root);
        gradient_pixels(c, root, radial);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, copy;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = root;
        FVM_Channel *ch = 0;
        if (!(fvm_channel_create(&ch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        gradient_pixels(other, copy.object, radial);
        fvm_channel_destroy(ch);
        unsigned char *snapshot = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(c, &value, &snapshot, &size) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_deserialize(other, snapshot, size, &copy) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(snapshot);
        if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        gradient_pixels(other, copy.object, radial);
        if (variant == 5)
        {
            FVM_Channel *file = 0;
            FVM_Result opened = fvm_channel_open_file("flashvm-gradient-test.bin", "w", &file);
            if (opened == FVM_OK)
            {
                if (!(fvm_channel_send_value(c, file, &value, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_channel_destroy(file);
                if (!(fvm_channel_open_file("flashvm-gradient-test.bin", "r", &file) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_channel_receive_value(other, file, &copy, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_channel_destroy(file);
                if (!(std::remove("flashvm-gradient-test.bin") == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                gradient_pixels(other, copy.object, radial);
            }
            else
            {
                if (!(opened == FVM_UNSUPPORTED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    size_t header = 0;
    Bytes payload = gradient_shape(0x10, 32, &header);
    FVM_SWFShape decoded;
    for (size_t n = 0; n < payload.size(); ++n)
    {
        if (!(fvm_swf_read_shape(payload.begin(), n, 32, decoded) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned int k = 0; k < 3; ++k)
    {
        Bytes bad = payload;
        bad[header] = k == 0 ? 0 : k == 1 ? 9 : 0x12;
        if (!(fvm_swf_read_shape(bad.begin(), bad.size(), 32, decoded) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes unsorted = payload;
    unsorted[header + 1] = 200;
    unsorted[header + 6] = 100;
    if (!(fvm_swf_read_shape(unsorted.begin(), unsorted.size(), 32, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes colored, empty;
    tag(colored, 32, gradient_shape());
    tag(colored, 26, place(2 | 4 | 8, 1, 9, 0, 0));
    tag(colored, 1, empty);
    tag(colored, 0, empty);
    FVM_Object *root = load(c, colored, 1);
    tick(c, root);
    unsigned char pixels[16 * 16 * 4];
    std::memset(pixels, 0, sizeof(pixels));
    FVM_RasterSurface surface = {pixels, 16, 16, 64};
    FVM_Result rendered = fvm_movieclip_render(c, root, &surface);
    if (rendered == FVM_OK)
    {
        unsigned char *p = pixels + 4 * (8 * 16 + 8);
        if (!(p[0] == 0 && p[1] == 255 && p[2] == 0 && p[3] == 128))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(rendered == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
