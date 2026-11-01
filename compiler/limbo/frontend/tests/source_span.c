#include "source_position.h"
#include <stdio.h>


/* Exercise every ordered/reversed pair against independent fixture coordinates.
 * UTF-8 and CRLF interiors are forbidden even for empty spans. Outputs must remain
 * unchanged on refusal, including when malformed bytes follow both endpoints. */
int main(void)
{
    static const char text[] = "A\t\316\273\r\n\360\237\230\200e\314\201";
    static const size_t lines[] = {1,1,1,0,1,0,2,0,0,0,2,2,0,2};
    static const size_t columns[] = {1,2,3,0,4,0,1,0,0,0,2,3,0,4};
    size_t begin;
    size_t end;
    for (begin = 0; begin <= sizeof(text); ++begin)
    {
        for (end = 0; end <= sizeof(text); ++end)
        {
            LimboSourceSpan out = {{91,92,93},{94,95,96}};
            int expected = begin <= end && end < sizeof(text) && lines[begin] && lines[end];
            int accepted = limbo_source_span(text,sizeof(text)-1,begin,end,&out);
            if (accepted != expected ||
                (accepted && (out.begin.byte_offset != begin || out.end.byte_offset != end ||
                 out.begin.line != lines[begin] || out.begin.column != columns[begin] ||
                 out.end.line != lines[end] || out.end.column != columns[end])) ||
                (!accepted && (out.begin.byte_offset != 91 || out.begin.line != 92 || out.begin.column != 93 ||
                 out.end.byte_offset != 94 || out.end.line != 95 || out.end.column != 96)))
            {
                fprintf(stderr,"span mismatch: %lu..%lu\n",(unsigned long)begin,(unsigned long)end);
                return 1;
            }
        }
    }
    {
        const char invalid[] = {'a','b',(char)0xff};
        LimboSourceSpan out = {{91,92,93},{94,95,96}};
        if (limbo_source_span(invalid,sizeof(invalid),0,1,&out) || out.begin.byte_offset != 91 ||
            limbo_source_span(NULL,1,0,0,&out) || limbo_source_span(text,sizeof(text)-1,0,0,NULL) ||
            !limbo_source_span(NULL,0,0,0,&out) || out.begin.line != 1 || out.end.column != 1)
        {
            fputs("span snapshot/empty/refusal contract failed\n",stderr);
            return 1;
        }
    }
    puts("PASS source spans: all endpoint pairs, UTF-8/CRLF interiors, EOF, empty, atomic refusal");
    return 0;
}
