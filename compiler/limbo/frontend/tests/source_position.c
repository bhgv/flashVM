#include "source_position.h"
#include <stdio.h>
#include <string.h>


/* Check exact coordinates and transactional refusal at every byte boundary of
 * a borrowed fixture. Expected zero line marks a forbidden interior boundary. */
static int fixture(const char *text, size_t size, const size_t *lines, const size_t *columns)
{
    size_t offset;
    for (offset = 0; offset <= size; ++offset)
    {
        LimboSourcePosition result = {99, 98, 97};
        int accepted = limbo_source_position(text, size, offset, &result);
        if (accepted != (lines[offset] != 0) ||
            (accepted && (result.byte_offset != offset || result.line != lines[offset] || result.column != columns[offset])) ||
            (!accepted && (result.byte_offset != 99 || result.line != 98 || result.column != 97)))
        {
            fprintf(stderr, "source position mismatch at byte %lu\n", (unsigned long)offset);
            return 0;
        }
    }
    return 1;
}


/* Independently encode every Unicode scalar and test all interior byte offsets.
 * No scanner/decoder helper is shared with the implementation under test. */
static int all_scalars(void)
{
    unsigned int value;
    for (value = 1; value <= 1114111U; ++value)
    {
        char encoded[4];
        size_t width;
        size_t offset;
        LimboSourcePosition result;
        if ((value >= 55296U && value <= 57343U) || value == 10 || value == 13)
        {
            continue;
        }
        if (value < 128U)
        {
            width = 1;
            encoded[0] = (char)value;
        }
        else if (value < 2048U)
        {
            width = 2;
            encoded[0] = (char)(192U | (value >> 6));
            encoded[1] = (char)(128U | (value & 63U));
        }
        else if (value < 65536U)
        {
            width = 3;
            encoded[0] = (char)(224U | (value >> 12));
            encoded[1] = (char)(128U | ((value >> 6) & 63U));
            encoded[2] = (char)(128U | (value & 63U));
        }
        else
        {
            width = 4;
            encoded[0] = (char)(240U | (value >> 18));
            encoded[1] = (char)(128U | ((value >> 12) & 63U));
            encoded[2] = (char)(128U | ((value >> 6) & 63U));
            encoded[3] = (char)(128U | (value & 63U));
        }
        for (offset = 0; offset <= width; ++offset)
        {
            int valid = offset == 0 || offset == width;
            if (limbo_source_position(encoded,width,offset,&result) != valid ||
                (valid && (result.line != 1 || result.column != (offset ? 2U : 1U))))
            {
                fprintf(stderr,"Unicode scalar boundary failed: U+%X offset %lu\n",value,(unsigned long)offset);
                return 0;
            }
        }
    }
    return 1;
}


/* Execute independent Unicode/newline fixtures and malformed-source refusals.
 * No parser is mocked; the public pure coordinate implementation is linked. */
int main(void)
{
    const char source[] = "a\t\316\273\360\237\230\200\r\nb\rc\ne\314\201";
    const size_t lines[] = {1,1,1,0,1,0,0,0,1,0,2,2,3,3,4,4,0,4};
    const size_t columns[] = {1,2,3,0,4,0,0,0,5,0,1,2,1,2,1,2,0,3};
    const unsigned char bad[][5] = {
        {0,0,0,0,0}, {192,128,0,0,0}, {237,160,128,0,0},
        {244,144,128,128,0}, {240,159,152,0,0}, {128,0,0,0,0}
    };
    const size_t lengths[] = {1,2,3,4,3,1};
    size_t i;
    LimboSourcePosition position = {7,8,9};
    if (!all_scalars() || !fixture(source, sizeof(source)-1, lines, columns) ||
        !limbo_source_position(NULL,0,0,&position) || position.line != 1 || position.column != 1)
    {
        return 1;
    }
    for (i = 0; i < sizeof(lengths)/sizeof(lengths[0]); ++i)
    {
        position.byte_offset = 7;
        position.line = 8;
        position.column = 9;
        if (limbo_source_position((const char *)bad[i],lengths[i],0,&position) ||
            position.byte_offset != 7 || position.line != 8 || position.column != 9)
        {
            fprintf(stderr,"malformed source was admitted or changed output\n");
            return 1;
        }
    }
    if (limbo_source_position("x",1,2,&position) ||
        limbo_source_position("x",2097153U,0,&position) ||
        limbo_source_position(NULL,1,0,&position) ||
        limbo_source_position("x",1,0,NULL))
    {
        return 1;
    }
    puts("PASS normalized source coordinates: UTF-8, tabs, CRLF/CR/LF, combining, EOF and atomic refusal");
    return 0;
}
