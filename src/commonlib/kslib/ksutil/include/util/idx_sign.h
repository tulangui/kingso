#ifndef IDX_SIGN_H
#define IDX_SIGN_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/*
 * func : make a 64 bit string signature
 *
 * args : str, len, input string and its length
 *
 * ret  : 64 bit signature of the string, 
 *        0L indicates wrong.
 */
uint64_t idx_sign64(const char *str, const unsigned int len);

#ifdef __cplusplus
}
#endif

#endif
