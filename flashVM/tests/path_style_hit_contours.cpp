#include <stdio.h>
#include <stdlib.h>
#include "path_style_hit_fixture.hpp"


int main()
{
    for (unsigned cap = 0; cap < 3; ++cap)
    {
        for (unsigned join = 0; join < 2; ++join)
        {
            FVM_Path *p = hit_line(4, 4, 12, 4);
            if (!(hit_add(p, FVM_PATH_LINE, 12, 12) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(hit_add(p, FVM_PATH_CLOSE) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(selected_hit(p, 4, 4, cap, join)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!selected_hit(p, 1, 1, cap, join)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(hit_add(p, FVM_PATH_MOVE, 30, 8) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(hit_add(p, FVM_PATH_LINE, 40, 8) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(selected_hit(p, 29, 8, cap, join) == (cap != 1)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!selected_hit(p, 22, 8, cap, join)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_path_destroy(p);
            p = hit_line(2, 8, 2, 8);
            if (!(!selected_hit(p, 2, 8, cap, join)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_path_destroy(p);
        }
    }
    return 0;
}
