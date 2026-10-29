#include "error.h"
#include <stdio.h>

void debugPrintError(char const *str) {
    (void)fprintf(stderr, "[DEBUG]: %s", str);
}
