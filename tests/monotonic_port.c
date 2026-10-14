/* The selected real platform adapter supplies finite, nondecreasing elapsed clock samples. */
#include "../ports/monotonic_port.h"
#include <assert.h>
#include <float.h>
int main(void)
{
    double first,second;
    assert(!fvm_port_monotonic_ms(0));
    assert(fvm_port_monotonic_ms(&first) && first>=0 && first<=DBL_MAX);
    assert(fvm_port_monotonic_ms(&second) && second>=first && second<=DBL_MAX);
    return 0;
}
