/* The selected real platform adapter supplies finite, nondecreasing elapsed clock samples. */
#include <stdio.h>
#include <stdlib.h>
#include "../ports/monotonic_port.h"
#include <float.h>


int main(void)
{
    double first, second;
    if (!(!fvm_port_monotonic_ms(0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_port_monotonic_ms(&first) && first >= 0 && first <= DBL_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_port_monotonic_ms(&second) && second >= first && second <= DBL_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
