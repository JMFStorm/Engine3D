#include "j_assert.h"

void assert_true(bool assertion, const char* assertion_title, const char* file, const char* func, int line)
{
	if (!assertion)
	{
		printf("ERROR: Assertion (%s) failed in: %s at function: (%s), lilne: %d.\n", assertion_title, file, func, line);
		exit(1);
	}
}