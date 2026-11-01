#include "source_position.h"
#include <stdio.h>


/* Resolve the EOF of one borrowed UTF-8 snapshot. No parser or file is opened.
 * Exit 0 means exact expected physical coordinates; 1 reports refusal/mismatch. */
int main(void)
{
    /* The first line contains a tab, lambda and astral smile; CRLF is one break.
     * The second line contains e plus a combining accent: two Unicode scalars. */
    const char source[] = "\t\316\273\360\237\230\200\r\ne\314\201";
    LimboSourcePosition position;
    if (!limbo_source_position(source,sizeof(source)-1,sizeof(source)-1,&position))
    {
        fputs("Invalid source boundary\n",stderr);
        return 1;
    }
    printf("byte=%lu line=%lu scalar-column=%lu\n",
           (unsigned long)position.byte_offset,(unsigned long)position.line,
           (unsigned long)position.column);
    return position.line == 2 && position.column == 3 ? 0 : 1;
}
