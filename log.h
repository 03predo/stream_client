#include <stdio.h>

#define LOG(X...) printf(X)

#ifdef DEBUG
#define LOGD(X...) LOG(X)
#else
#define LOGD(X...)
#endif
