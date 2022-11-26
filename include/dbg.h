#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STRICT_ERROR_EXIT
#ifdef STRICT_ERROR_EXIT
    #define dbg_error_trace(fmt, ...)                                           \
        do {                                                                    \
            fprintf(stderr, "[%s|%03d|%s] ", __FILE__, __LINE__, __FUNCTION__); \
            fprintf(stderr, ##__VA_ARGS__);                                     \
            exit(EXIT_FAILURE);                                                 \
        } while (0)
#else
    #define dbg_error_trace(fmt, ...)                                           \
        do {                                                                    \
            fprintf(stderr, "[%s|%03d|%s] ", __FILE__, __LINE__, __FUNCTION__); \
            fprintf(stderr, ##__VA_ARGS__);                                     \
        } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_H__ */