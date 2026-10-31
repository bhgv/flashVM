implement Idesourcedomain;
include "idesourcedomain.m";
include "idemdi.m";


# Return a stable tool/navigation family from copied suffix and graphics flag.
# Unknown or untitled sources use Limbo; suffix comparisons ignore ASCII case.
family(path: string, graphics: int): int
{
    if (graphics) {
        return Idemdi->Graphics;
    }
    suffix := "";
    for (index := len path - 1; index >= 0; index--) {
        if (path[index] == '.') {
            suffix = path[index:];
            break;
        }
        if (path[index] == '/') {
            break;
        }
    }
    for (index = 0; index < len suffix; index++) {
        if (suffix[index] >= 'A' && suffix[index] <= 'Z') {
            suffix[index] += 'a' - 'A';
        }
    }
    if (suffix == ".as" || suffix == ".as1" || suffix == ".as2" || suffix == ".as3") {
        return Idemdi->ActionScript;
    }
    return Idemdi->Source;
}
