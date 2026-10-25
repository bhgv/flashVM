#include <stdio.h>
#include <stdlib.h>
#include "path_style_hit_fixture.hpp"


int main()
{
    for (unsigned cap = 0; cap < 3; ++cap)
    {
        for (unsigned join = 0; join < 2; ++join)
        {
            FVM_Path *p = hit_line();
            if (!(selected_hit(p, 8, 8, cap, join)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!selected_hit(p, 8, 10.01, cap, join)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(selected_hit(p, 1, 8, cap, join) == (cap != 1)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(selected_hit(p, 15, 8, cap, join) == (cap != 1)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(selected_hit(p, .1, 6.1, cap, join) == (cap == 2)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(selected_hit(p, 15.9, 9.9, cap, join) == (cap == 2)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(selected_hit(p, 2, 10, cap, join)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!selected_hit(p, -.01, 8, cap, join)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            int hit = 0;
            if (!(fvm_path_hit_test(p, .1, 6.1, 4, 0, &hit) == FVM_OK && !hit))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_path_destroy(p);
        }
    }
    return 0;
}
