/* Copyright (c) 2026 bhgv
 * EN: C89 prefix oracle endpoint, independent Python supplies expected wire bytes.
 * RU: C89-интерфейс проверки префикса; независимый Python задаёт ожидаемые байты.
 */
#include "flashvm/sorder.h"
#include <stdio.h>


/* EN/RU: Read pairs of uint32 limbs from stdin, emit canonical prefix hex; no heap or VM.
 * Читать пары uint32-частей из stdin, выводить hex префикса; без heap и VM.
 */
int main(void)
{
    FVM_SOrdinal value; /* Input portable magnitude. */
    unsigned char bytes[9]; /* Largest compact prefix. */
    size_t used; /* Exact encoded extent. */
    size_t i; /* Output byte cursor. */
    while (scanf("%lx %lx", &value.high, &value.low) == 2)
    {
        if (fvm_sordinal_write(&value, bytes, sizeof(bytes), &used) != FVM_S_OK)
        {
            return 1;
        }
        for (i = 0; i < used; i++)
        {
            printf("%02x", bytes[i]);
        }
        putchar('\n');
    }
    return ferror(stdin) || ferror(stdout);
}
