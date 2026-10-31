implement Iderecent;
include "iderecent.m";


# Return a fresh bounded history, deduplicating exact paths without touching disk.
# Empty paths leave the copied history unchanged. Caller owns all returned values.
remember(paths: array of string, path: string): array of string
{
    result := array[4] of string;
    used := 0;
    if (path != "") {
        result[used++] = path;
    }
    for (index := 0; index < len paths && used < len result; index++) {
        if (paths[index] != "" && paths[index] != path) {
            result[used++] = paths[index];
        }
    }
    return result;
}


# Encode display-only labels. Replace controls before adding separators; preserve
# the original full paths solely in the application owner for Open admission.
labels(paths: array of string): string
{
    result := "";
    for (index := 0; index < 4; index++) {
        name := "(empty)";
        if (index < len paths && paths[index] != "") {
            name = paths[index];
        }
        if (len name > 28) {
            name = name[len name - 28:];
        }
        for (scalar := 0; scalar < len name; scalar++) {
            value := name[scalar];
            if (value < 32 || value == 127 || value == 16r85 || value == 16r2028 || value == 16r2029) {
                name[scalar] = ' ';
            }
        }
        result += string (index + 1) + ": " + name + "\n";
    }
    return result;
}
