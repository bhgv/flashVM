#include <stdio.h>
#include <stdlib.h>
#include "path_style_hit_fixture.hpp"


int main()
{
    for (unsigned degree = 2; degree <= 3; ++degree)
    {
        for (unsigned cap = 0; cap < 3; ++cap)
        {
            for (unsigned join = 0; join < 2; ++join)
            {
                FVM_Path *p = 0;
                if (!(fvm_path_create(&p) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(hit_add(p, FVM_PATH_MOVE, 2, 8) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (degree == 2)
                {
                    if (!(hit_add(p, FVM_PATH_QUADRATIC, 8, 8, 14, 8) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(hit_add(p, FVM_PATH_CUBIC, 6, 8, 10, 8, 14, 8) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(selected_hit(p, 1, 8, cap, join) == (cap != 1)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(selected_hit(p, .1, 6.1, cap, join) == (cap == 2)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(selected_hit(p, 8, 9.9, cap, join)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_path_destroy(p);
                if (!(fvm_path_create(&p) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(hit_add(p, FVM_PATH_MOVE, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (degree == 2)
                {
                    if (!(hit_add(p, FVM_PATH_QUADRATIC, 8, 16, 16, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(hit_add(p, FVM_PATH_CUBIC, 0, 16, 16, 16, 16, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(selected_hit(p, 8, degree == 2 ? 8 : 12, cap, join)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!selected_hit(p, 8, 20, cap, join)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_path_destroy(p);
            }
        }
    }
    return 0;
}
