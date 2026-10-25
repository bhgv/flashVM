#include "as1/frontend/frontend.h"
#include "as2/frontend/frontend.h"
#include "as3/frontend/frontend.h"
#include "limbo/frontend/frontend.h"


int main(void)
{
    int i;
    for (i = 0; i < 32; ++i)
    {
        if (!as1_check("", 0, 0) || !as2_check("", 0, 0) || !as3_check("", 0, 0) || !limbo_check("implement A;", 12, 0))
        {
            return 1;
        }
        if (as1_check("@", 1, 0) || as2_check("@", 1, 0) || as3_check("@", 1, 0) || limbo_check("@", 1, 0))
        {
            return 2;
        }
    }
    return 0;
}
