#include <string.h>
#include <openssl/md5.h>
#include "util/idx_sign.h"
#include "util/common.h"

/*
 * func : make a 64 bit string signature
 *
 * args : str, len, input string and its length
 *
 * ret  : 64 bit signature of the string,
 *        0L indicates wrong.
 */
uint64_t idx_sign64(const char *str, const unsigned int len)
{
	if ( unlikely(NULL == str || 0 == len) ) {
		//TERR("parameter error: str %p, len %u", str, len);
		return 0L;
	}

	if ( len <= 8 ) {
		// strlen小于等于8的，直接用str原文作为签名，不足8byte的补'\0'
		// 方便从签名查看token原文
		uint64_t sign = 0L;
		memcpy(&sign, str, len);

		return sign;
	} else {
		uint64_t sign[2];
		MD5((const unsigned char*)str, len, (unsigned char*)sign);

		return sign[0] + sign[1];
	}
}
