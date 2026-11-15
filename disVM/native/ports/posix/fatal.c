/* Copyright (c) 2026 bhgv
 * EN: Original library invariant failure endpoint, independent of any VM.
 * RU: Library invariant failure endpoint, независимо от VM.
 */
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Report an upstream invariant and terminate; no allocation or formatter recursion.
 * Сообщить upstream invariant и завершить; без allocation/formatter recursion. */
void
_assert(char *message)
{
	fprintf(stderr, "Inferno library invariant: %s\n", message);
	abort();
}
