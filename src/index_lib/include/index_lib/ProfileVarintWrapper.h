/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ProfileVarintWrapper.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: 对多值整数进行Varint编解码操作的包装类 $
 ********************************************************************/

#ifndef KINGSO_INDEXLIB_PROFILEVARINTWRAPPER_H_
#define KINGSO_INDEXLIB_PROFILEVARINTWRAPPER_H_

#include <stdint.h>
#include <string.h>

#include "varint.h"
#include "IndexLib.h"

namespace index_lib
{


class ProfileVarintWrapper
{
public:
    ProfileVarintWrapper();
    ~ProfileVarintWrapper();

    /**
     * 对一组UINT32整数进行varint编码
     * @param  originNumArr   原始uint32整数数组
     * @param  num            原始整数数组大小
     * @param  encodeBuf      编码值输出的目标buffer
     * @param  buffSize       编码值输出buffer的空间大小
     * @return                负值，ERROR; 正数, 编码后的空间大小
     */
    inline int encode(uint32_t *originNumArr, uint32_t num, uint8_t *encodeBuf, uint32_t buffSize)
    {
        if (unlikely(originNumArr == NULL || num == 0
                    || encodeBuf == NULL  || buffSize < (5 * num)))
        {
            return KS_EINVAL;
        }

        uint32_t group_count  = num / 4;
        uint32_t varint_count = num % 4;
        uint8_t  *begin       = encodeBuf;
        uint32_t *targetArr   = originNumArr;

        for(uint32_t group = 0; group < group_count; ++group) {
            begin = group_varint_encode_uint32(targetArr, begin);
            targetArr += 4;
        }

        for(uint32_t var = 0; var < varint_count; ++var) {
            begin = varint_encode_uint32(*targetArr, begin);
            ++targetArr;
        }
        return (begin - encodeBuf);
    }

    /**
     * 对一组INT32整数进行varint编码
     * @param  originNumArr   原始int32整数数组
     * @param  num            原始整数数组大小
     * @param  encodeBuf      编码值输出的目标buffer
     * @param  buffSize       编码值输出buffer的空间大小
     * @return                负值，ERROR; 正数, 编码后的空间大小
     */
    inline int encode(int32_t *originNumArr, uint32_t num, uint8_t *encodeBuf, uint32_t buffSize)
    {
        if (unlikely(originNumArr == NULL || num == 0
                    || encodeBuf == NULL  || buffSize < (5 * num)))
        {
            return KS_EINVAL;
        }

        for(uint32_t pos = 0; pos < num; ++pos) {
            originNumArr[pos] = (int32_t)zigZag_encode32(originNumArr[pos]) ;
        }

        uint32_t group_count  = num / 4;
        uint32_t varint_count = num % 4;
        uint8_t  *begin       = encodeBuf;
        uint32_t *targetArr   = (uint32_t *)originNumArr;

        for(uint32_t group = 0; group < group_count; ++group) {
            begin = group_varint_encode_uint32(targetArr, begin);
            targetArr += 4;
        }

        for(uint32_t var = 0; var < varint_count; ++var) {
            begin = varint_encode_uint32(*targetArr, begin);
            ++targetArr;
        }
        return (begin - encodeBuf);
    }

    /**
     * 对一组UINT64整数进行varint编码
     * @param  originNumArr   原始uint64整数数组
     * @param  num            原始整数数组大小
     * @param  encodeBuf      编码值输出的目标buffer
     * @param  buffSize       编码值输出buffer的空间大小
     * @return                负值，ERROR; 正数, 编码后的空间大小
     */
    inline int encode(uint64_t *originNumArr, uint32_t num, uint8_t *encodeBuf, uint32_t buffSize)
    {
        if (unlikely(originNumArr == NULL || num == 0
                    || encodeBuf == NULL  || buffSize < (10 * num)))
        {
            return KS_EINVAL;
        }

        uint32_t group_count  = (num / 2);
        uint32_t varint_count = (num % 2);
        uint8_t  *begin       = encodeBuf;
        uint64_t *targetArr   = (uint64_t *)originNumArr;

        for(uint32_t group = 0; group < group_count; ++group) {
            begin = group_varint_encode_uint64(targetArr, begin);
            targetArr += 2;
        }

        if (varint_count == 1) {
            GRPVARINT64 tmp;
            tmp.v64 = (*targetArr);
            begin = varint_encode_uint32(tmp.v32[0], begin);
            begin = varint_encode_uint32(tmp.v32[1], begin);
        }
        return (begin - encodeBuf);
    }

    /**
     * 对一组INT64整数进行varint编码
     * @param  originNumArr   原始int64整数数组
     * @param  num            原始整数数组大小
     * @param  encodeBuf      编码值输出的目标buffer
     * @param  buffSize       编码值输出buffer的空间大小
     * @return                负值，ERROR; 正数, 编码后的空间大小
     */
    inline int encode(int64_t *originNumArr, uint32_t num, uint8_t *encodeBuf, uint32_t buffSize)
    {
        if (unlikely(originNumArr == NULL || num == 0
                    || encodeBuf == NULL  || buffSize < (10 * num)))
        {
            return KS_EINVAL;
        }

        for(uint32_t pos = 0; pos < num; ++pos) {
            originNumArr[pos] = (int64_t)zigZag_encode64(originNumArr[pos]);
        }

        uint32_t group_count  = (num / 2);
        uint32_t varint_count = (num % 2);
        uint8_t  *begin       = encodeBuf;
        uint64_t *targetArr   = (uint64_t *)originNumArr;

        for(uint32_t group = 0; group < group_count; ++group) {
            begin = group_varint_encode_uint64(targetArr, begin);
            targetArr += 2;
        }

        if (varint_count == 1) {
            GRPVARINT64 tmp;
            tmp.v64 = (*targetArr);
            begin = varint_encode_uint32(tmp.v32[0], begin);
            begin = varint_encode_uint32(tmp.v32[1], begin);
        }
        return (begin - encodeBuf);
    }


    /**
     * 对一组UINT32整数进行varint解码，获取下标对应的目标值
     * @param  encodeBuf      编码值对应的存储空间
     * @param  num            原始整数数组大小
     * @param  index          原始整数数组中的目标整数下标
     * @param  value [out]    返回目标整数值
     * @return                负值，ERROR; 0, OK
     */
    inline int decode(const uint8_t *encodeBuf, uint32_t num, uint32_t index, uint32_t &value)
    {
        if (unlikely(encodeBuf == NULL || num == 0 || index >= num)) {
            return KS_EINVAL;
        }

        uint32_t valueArr[4]  = {0};
        uint32_t group_count  = num / 4;
        uint32_t index_group  = index / 4;
        uint32_t index_pos    = index % 4;
        const uint8_t * begin = encodeBuf;

        for (uint32_t group = 0; group < index_group; ++group) {
            begin = group_varint_decode_uint32(begin, valueArr);
        }

        if (group_count > 0 && index_group < group_count) {
            begin = group_varint_decode_uint32(begin, valueArr);
            value = valueArr[index_pos];
        }
        else {
            uint32_t targetValue;
            for(uint32_t num = 0; num <= index_pos; ++num) {
                begin = varint_decode_uint32(begin, &targetValue);
                if (begin == NULL) {
                    return KS_EINVAL;
                }
            }
            value = targetValue;
        }
        return 0;
    }

    /**
     * 对一组INT32整数进行varint解码，获取下标对应的目标值
     * @param  encodeBuf      编码值对应的存储空间
     * @param  num            原始整数数组大小
     * @param  index          原始整数数组中的目标整数下标
     * @param  value [out]    返回目标整数值
     * @return                负值，ERROR; 0, OK
     */
    inline int decode(const uint8_t *encodeBuf, uint32_t num, uint32_t index, int32_t &value)
    {
        if (unlikely(encodeBuf == NULL || num == 0 || index >= num)) {
            return KS_EINVAL;
        }

        uint32_t valueArr[4]  = {0};
        uint32_t group_count  = num / 4;
        uint32_t index_group  = index / 4;
        uint32_t index_pos    = index % 4;
        const uint8_t * begin = encodeBuf;

        for (uint32_t group = 0; group < index_group; ++group) {
            begin = group_varint_decode_uint32(begin, valueArr);
        }

        if (group_count > 0 && index_group < group_count) {
            begin = group_varint_decode_uint32(begin, valueArr);
            value = zigZag_decode32(valueArr[index_pos]);
        }
        else {
            uint32_t zigZag_value = 0;
            for(uint32_t num = 0; num <= index_pos; ++num) {
                begin = varint_decode_uint32(begin, &zigZag_value);
                if (begin == NULL) {
                    return KS_EINVAL;
                }
            }
            value = zigZag_decode32(zigZag_value);
        }
        return 0;
    }

    /**
     * 对一组UINT64整数进行varint解码，获取下标对应的目标值
     * @param  encodeBuf      编码值对应的存储空间
     * @param  num            原始整数数组大小
     * @param  index          原始整数数组中的目标整数下标
     * @param  value [out]    返回目标整数值
     * @return                负值，ERROR; 0, OK
     */
    inline int decode(const uint8_t *encodeBuf, uint32_t num, uint32_t index, uint64_t &value)
    {
        if (unlikely(encodeBuf == NULL || num == 0 || index >= num)) {
            return KS_EINVAL;
        }

        uint64_t valueArr[2]  = {0};
        uint32_t group_count  = (num / 2);
        uint32_t index_group  = (index / 2);
        uint32_t index_pos    = (index * 2) % 4;
        const uint8_t * begin = encodeBuf;

        for (uint32_t group = 0; group < index_group; ++group) {
            begin = group_varint_decode_uint64(begin, valueArr);
        }

        if (group_count > 0 && index_group < group_count) {
            begin = group_varint_decode_uint64(begin, valueArr);
            value = valueArr[index_pos / 2];
        }
        else {
            GRPVARINT64 tmp;
            begin = varint_decode_uint32(begin, &(tmp.v32[0]));
            if (begin == NULL) {
                return KS_EINVAL;
            }

            begin = varint_decode_uint32(begin, &(tmp.v32[1]));
            if (begin == NULL) {
                return KS_EINVAL;
            }
            value = tmp.v64;
        }

        return 0;
    }


    /**
     * 对一组INT64整数进行varint解码，获取下标对应的目标值
     * @param  encodeBuf      编码值对应的存储空间
     * @param  num            原始整数数组大小
     * @param  index          原始整数数组中的目标整数下标
     * @param  value [out]    返回目标整数值
     * @return                负值，ERROR; 0, OK
     */
    inline int decode(const uint8_t *encodeBuf, uint32_t num, uint32_t index, int64_t &value)
    {
        if (unlikely(encodeBuf == NULL || num == 0 || index >= num)) {
            return KS_EINVAL;
        }

        uint64_t valueArr[2]  = {0};
        uint32_t group_count  = (num / 2);
        uint32_t index_group  = (index / 2);
        uint32_t index_pos    = (index * 2) % 4;
        const uint8_t * begin = encodeBuf;

        for (uint32_t group = 0; group < index_group; ++group) {
            begin = group_varint_decode_uint64(begin, valueArr);
        }

        if (group_count > 0 && index_group < group_count) {
            begin = group_varint_decode_uint64(begin, valueArr);
            value = zigZag_decode64(valueArr[index_pos / 2]);
        }
        else {
            GRPVARINT64 tmp;
            begin = varint_decode_uint32(begin, &(tmp.v32[0]));
            if (begin == NULL) {
                return KS_EINVAL;
            }

            begin = varint_decode_uint32(begin, &(tmp.v32[1]));
            if (begin == NULL) {
                return KS_EINVAL;
            }
            value = zigZag_decode64(tmp.v64);
        }
        return 0;
    }

    /**
     * 对一组INT32整数进行varint解码
     * @param  encodeBuf      编码值对应的存储空间
     * @param  num            原始整数数组大小
     * @param  valueBuf [out] 解码值存储的目标数组空间
     * @param  size           解码目标数组的大小
     * @return                负值，ERROR;  else: 压缩块的长度
     */
    inline int decode(const uint8_t *encodeBuf, uint32_t num, int32_t *valueBuf, uint32_t size)
    {
        if (unlikely(encodeBuf == NULL || valueBuf == NULL || num == 0 || size < num)) {
            return KS_EINVAL;
        }

        uint32_t valueArr[4]  = {0};
        uint32_t group_count  = num / 4;
        uint32_t varint_pos   = num % 4;
        const uint8_t * begin = encodeBuf;
        uint32_t target_pos   = 0;

        for (uint32_t group = 0; group < group_count; ++group) {
            begin = group_varint_decode_uint32(begin, valueArr);
            valueBuf[target_pos]   = zigZag_decode32(valueArr[0]);
            valueBuf[target_pos + 1] = zigZag_decode32(valueArr[1]);
            valueBuf[target_pos + 2] = zigZag_decode32(valueArr[2]);
            valueBuf[target_pos + 3] = zigZag_decode32(valueArr[3]);
            target_pos += 4;
        }

        uint32_t zigZag_value = 0;
        for(uint32_t num = 0; num < varint_pos; ++num) {
            begin = varint_decode_uint32(begin, &zigZag_value);
            if (unlikely(begin == NULL)) {
                return KS_EINVAL;
            }
            valueBuf[target_pos++] = zigZag_decode32(zigZag_value);
        }

        return (begin - encodeBuf);
    }

    /**
     * 对一组UINT32整数进行varint解码
     * @param  encodeBuf      编码值对应的存储空间
     * @param  num            原始整数数组大小
     * @param  valueBuf [out] 解码值存储的目标数组空间
     * @param  size           解码目标数组的大小
     * @return                负值，ERROR;  else: 压缩块的长度
     */
    inline int decode(const uint8_t *encodeBuf, uint32_t num, uint32_t *valueBuf, uint32_t size)
    {
        if (unlikely(encodeBuf == NULL || valueBuf == NULL || num == 0 || size < num)) {
            return KS_EINVAL;
        }

        uint32_t valueArr[4]  = {0};
        uint32_t group_count  = num / 4;
        uint32_t varint_pos   = num % 4;
        const uint8_t * begin = encodeBuf;
        uint32_t target_pos   = 0;

        for (uint32_t group = 0; group < group_count; ++group) {
            begin = group_varint_decode_uint32(begin, valueArr);
            valueBuf[target_pos]   = valueArr[0];
            valueBuf[target_pos + 1] = valueArr[1];
            valueBuf[target_pos + 2] = valueArr[2];
            valueBuf[target_pos + 3] = valueArr[3];
            target_pos += 4;
        }

        uint32_t zigZag_value = 0;
        for(uint32_t num = 0; num < varint_pos; ++num) {
            begin = varint_decode_uint32(begin, &zigZag_value);
            if (unlikely(begin == NULL)) {
                return KS_EINVAL;
            }
            valueBuf[target_pos++] = zigZag_value;
        }

        return (begin - encodeBuf);
    }

    /**
     * 对一组INT64整数进行varint解码
     * @param  encodeBuf      编码值对应的存储空间
     * @param  num            原始整数数组大小
     * @param  valueBuf [out] 解码目标输出数组
     * @param  size           解码目标数组大小
     * @return                负值，ERROR;  else: 压缩块的长度
     */
    inline int decode(const uint8_t *encodeBuf, uint32_t num, int64_t* valueBuf, uint32_t size)
    {
        if (unlikely(encodeBuf == NULL || valueBuf == NULL || num == 0 || size < num)) {
            return KS_EINVAL;
        }

        uint64_t valueArr[2]  = {0};
        uint32_t group_count  = (num / 2);
        uint32_t varint_pos   = (num % 2);
        const uint8_t * begin = encodeBuf;
        uint32_t target_pos   = 0;

        for (uint32_t group = 0; group < group_count; ++group) {
            begin = group_varint_decode_uint64(begin, valueArr);
            valueBuf[target_pos]     = zigZag_decode64(valueArr[0]);
            valueBuf[target_pos + 1] = zigZag_decode64(valueArr[1]);
            target_pos += 2;
        }

        if (varint_pos == 1) {
            GRPVARINT64 tmp;
            begin = varint_decode_uint32(begin, &(tmp.v32[0]));
            if (begin == NULL) {
                return KS_EINVAL;
            }
            begin = varint_decode_uint32(begin, &(tmp.v32[1]));
            if (begin == NULL) {
                return KS_EINVAL;
            }
            valueBuf[target_pos] = zigZag_decode64(tmp.v64);
        }

        return (begin - encodeBuf);
    }

    /**
     * 对一组UINT64整数进行varint解码
     * @param  encodeBuf      编码值对应的存储空间
     * @param  num            原始整数数组大小
     * @param  valueBuf [out] 解码目标输出数组
     * @param  size           解码目标数组大小
     * @return                负值，ERROR;  else: 压缩块的长度
     */
    inline int decode(const uint8_t *encodeBuf, uint32_t num, uint64_t* valueBuf, uint32_t size)
    {
        if (unlikely(encodeBuf == NULL || valueBuf == NULL || num == 0 || size < num)) {
            return KS_EINVAL;
        }

        uint64_t valueArr[2]  = {0};
        uint32_t group_count  = (num / 2);
        uint32_t varint_pos   = (num % 2);
        const uint8_t * begin = encodeBuf;
        uint32_t target_pos   = 0;

        for (uint32_t group = 0; group < group_count; ++group) {
            begin = group_varint_decode_uint64(begin, valueArr);
            valueBuf[target_pos]     = valueArr[0];
            valueBuf[target_pos + 1] = valueArr[1];
            target_pos += 2;
        }

        if (varint_pos == 1) {
            GRPVARINT64 tmp;
            begin = varint_decode_uint32(begin, &(tmp.v32[0]));
            if (begin == NULL) {
                return KS_EINVAL;
            }
            begin = varint_decode_uint32(begin, &(tmp.v32[1]));
            if (begin == NULL) {
                return KS_EINVAL;
            }
            valueBuf[target_pos] = tmp.v64;
        }

        return (begin - encodeBuf);
    }

};
}

#endif //KINGSO_INDEXLIB_PROFILEGROUPVARINTWRAPPER_H_
