#ifndef WK_UTIL_H
#define WK_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define STRINGIFY(x) #x

#define red(x)  "\033[1m\033[31m"x

// TODO: Create log.c and get better logging.
#define nlog(x) fprintf(stderr, x"\n");
#define vlog(x, ...) fprintf(stderr, x"\n", __VA_ARGS__)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define failsafe(s) _failsafe(s, __FILE__, __LINE__, STRINGIFY(s))
static inline void* _failsafe(void* data, const char* file, const int line,
        const char* inst)
{
    if(!data) {
        fprintf(stderr, red("failsafe: %s at %s:%d \n"), inst, file, line);
        if(errno != 0)
            perror("failsafe: system");
        exit(EXIT_FAILURE);
    }
    return data;
}

#define fzalloc(s) failsafe(zalloc(s))
static inline void* zalloc(size_t size)
{
    return calloc(1, size);
}

#endif /* WK_UTIL_H */
