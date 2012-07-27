/** \file
 *******************************************************************
 * $Author: zhuangyuan $
 *
 * $LastChangedBy: zhuangyuan $
 *
 * $Revision: 6 $
 *
 * $LastChangedDate: 2011-03-15 17:31:24 +0800 (Tue, 15 Mar 2011) $
 *
 * $Id: hashfunc.h 6 2011-03-15 09:31:24Z zhuangyuan $
 *
 * $Brief: hash functions $
 *******************************************************************
 */

#ifndef HASH_FUNC_H
#define HASH_FUNC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief BKDR hash function for char* ending with '\\0'
     * @param str the string
     * @return uint64_t hash code
     */
    uint64_t bkdr_hash_func(const void *str);

    /**
     * @brief BKDR hash function for const char* with given length
     * @param str the string
     * @param len the length of input string
     * @return uint64_t hash code
     */
    uint64_t bkdr_hash_func_ex(const void *str, const int len);

    /**
     * @brief ELF hash function for char* ending with '\\0'
     * @param str the string
     * @return uint64_t hash code
     */
    uint64_t elf_hash_func(const void *str);

    /**
     * @brief ELF hash function for const char* with given length
     * @param str the string
     * @param len the length of input string
     * @return uint64_t hash code
     */
    uint64_t elf_hash_func_ex(const void *str, const int len);

    /**
     * @brief RS hash function for char* ending with '\\0'
     * @param str the string
     * @return uint64_t hash code
     */
    uint64_t rs_hash_func(const void *str);

    /**
     * @brief RS hash function for const char* with given length
     * @param str the string
     * @param len the length of input string
     * @return uint64_t hash code
     */
    uint64_t rs_hash_func_ex(const void *str, const int len);

    /**
     * @brief JS hash function for char* ending with '\\0'
     * @param str the string
     * @return uint64_t hash code
     */
    uint64_t js_hash_func(const void *str);

    /**
     * @brief JS hash function for const char* with given length
     * @param str the string
     * @param len the length of input string
     * @return uint64_t hash code
     */
    uint64_t js_hash_func_ex(const void *str, const int len);

    /**
     * @brief SDBM hash function for char* ending with '\\0'
     * @param str the string
     * @return uint64_t hash code
     */
    uint64_t sdbm_hash_func(const void *str);

    /**
     * @brief SDMB hash function for const char* with given length
     * @param str the string
     * @param len the length of input string
     * @return uint64_t hash code
     */
    uint64_t sdbm_hash_func_ex(const void *str, const int len);

    /**
     * @brief TIME33 hash function for char* ending with '\\0'
     * @param str the string
     * @return uint64_t hash code
     */
    uint64_t time33_hash_func(const void *str);

    /**
     * @brief TIME33 hash function for const char* with given length
     * @param str the string
     * @param len the length of input string
     * @return uint64_t hash code
     */
    uint64_t time33_hash_func_ex(const void *str, const int len);

#ifdef __cplusplus
}
#endif

#endif // HASH_FUNC_H

