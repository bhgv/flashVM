#include <stdio.h>
#include <stdlib.h>
#include "path_style_hit_fixture.hpp"


int main()
{
    for (unsigned closed = 0; closed < 2; ++closed)
    {
        for (unsigned curve = 0; curve < 3; ++curve)
        {
            FVM_Path *p = hit_line(2, 2, 14, 2);
            if (curve == 0)
            {
                if (!(hit_add(p, FVM_PATH_LINE, 8, 14) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (curve == 1)
            {
                if (!(hit_add(p, FVM_PATH_QUADRATIC, 14, 14, 2, 14) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (curve == 2)
            {
                if (!(hit_add(p, FVM_PATH_CUBIC, 18, 6, 18, 12, 2, 14) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (closed)
            {
                if (!(hit_add(p, FVM_PATH_CLOSE) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            for (int y = -2; y < 20; ++y)
            {
                for (int x = -2; x < 20; ++x)
                {
                    int legacy = 0;
                    if (!(fvm_path_hit_test(p, x + .371, y + .263, 4, 0, &legacy) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(legacy == selected_hit(p, x + .371, y + .263, 0)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            fvm_path_destroy(p);
        }
    }
    return 0;
}
