#include <stdio.h>

char lastError[256] = {0};

void setError(const char *szError)
{
    sprintf_s(lastError, 256, szError);
}

const char *egGetError()
{
    return lastError;
}
