/* Raw historical IDs independently protect stored Transform callbacks from schema renumbering. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/standard/transform_state.hpp"


int main()
{
    const unsigned long ids[] = {1400, 1401, 1402, 1410, 1411, 1412, 1413, 1414, 1420, 1421};
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
    {
        FVM_NativeFunction callback = fvm_transform_callback_from_id(ids[i]);
        if (!(callback && fvm_transform_callback_id(callback) == ids[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned j = 0; j < i; ++j)
        {
            if (!(callback != fvm_transform_callback_from_id(ids[j])))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    const unsigned long invalid[] = {0, 1399, 1403, 1409, 1415, 1419, 1422, ~0UL};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        if (!(!fvm_transform_callback_from_id(invalid[i])))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_transform_callback_id(0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
