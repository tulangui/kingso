/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ProfileMultiValueIterator.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: 多值字段读取接口所需的迭代器类 $
 * 注：该对象内部需要初始分配buffer空间,不要频繁创建迭代器对象
 ********************************************************************/

#ifndef KINGSO_INDEXLIB_PROFILEMULTIVALUEITERATOR_H_
#define KINGSO_INDEXLIB_PROFILEMULTIVALUEITERATOR_H_

#include <stdint.h>
#include "IndexLib.h"
#include "ProfileStruct.h"
#include "ProfileVarintWrapper.h"

namespace index_lib
{

/**
 * 多值字段值的迭代器类
 */
class ProfileMultiValueIterator
{
    public:
        /*
         * 构造函数
         */
        ProfileMultiValueIterator();

        /*
         * 析构函数
         */
        ~ProfileMultiValueIterator();

        /*
         * 获取下一个int8类型字段值
         * @return 下一个字段值
         */
        inline int8_t  nextInt8()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_INT8;
            }
            #endif
            return ((int8_t*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个int16类型字段值
         * @return 下一个字段值
         */
        inline int16_t nextInt16()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_INT16;
            }
            #endif
            return ((int16_t*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个int32类型字段值
         * @return 下一个字段值
         */
        inline int32_t nextInt32()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_INT32;
            }
            #endif

            if (unlikely(_pos == -1 && _compress)) {
                _wrapper.decode((uint8_t *)_targetAddr, _num, (int32_t *)_buffer, MAX_ENCODE_VALUE_NUM);
                _targetAddr = _buffer;
            }
            return ((int32_t*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个KV32类型字段
         * @return 下一个字段值
         */
        inline KV32 nextKV32()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                KV32 kv;
                kv.key   = INVALID_INT32;
                kv.value = INVALID_INT32;
                return kv;
            }
            #endif
            return ((KV32 *)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个int64类型字段值
         * @return 下一个字段值
         */
        inline int64_t nextInt64()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_INT64;
            }
            #endif

            if (unlikely(_pos == -1 && _compress)) {
                _wrapper.decode((uint8_t *)_targetAddr, _num, (int64_t *)_buffer, MAX_ENCODE_VALUE_NUM);
                _targetAddr = _buffer;
            }
            return ((int64_t*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个uint8类型字段值
         * @return 下一个字段值
         */
        inline uint8_t nextUInt8()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_UINT8;
            }
            #endif
            return ((uint8_t*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个uint16类型字段值
         * @return 下一个字段值
         */
        inline uint16_t nextUInt16()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_UINT16;
            }
            #endif
            return ((uint16_t*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个uint32类型字段值
         * @return 下一个字段值
         */
        inline uint32_t nextUInt32()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_UINT32;
            }
            #endif

            if (unlikely(_pos == -1 && _compress)) {
                _wrapper.decode((uint8_t *)_targetAddr, _num, (uint32_t *)_buffer, MAX_ENCODE_VALUE_NUM);
                _targetAddr = _buffer;
            }
            return ((uint32_t*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个uint64类型字段值
         * @return 下一个字段值
         */
        inline uint64_t nextUInt64()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_UINT64;
            }
            #endif

            if (unlikely(_pos == -1 && _compress)) {
                _wrapper.decode((uint8_t *)_targetAddr, _num, (uint64_t *)_buffer, MAX_ENCODE_VALUE_NUM);
                _targetAddr = _buffer;
            }
            return ((uint64_t*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个float类型字段值
         * @return 下一个字段值
         */
        inline float    nextFloat()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_FLOAT;
            }
            #endif
            return ((float*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个double类型字段值
         * @return 下一个字段值
         */
        inline double   nextDouble()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return INVALID_DOUBLE;
            }
            #endif
            return ((double*)_targetAddr)[++_pos];
        }

        /*
         * 获取下一个字符串类型字段值
         * @return 下一个字段值
         */
        inline const char* nextString()
        {
            #ifdef DEBUG
            if (unlikely((uint32_t)(_pos+1) >= _num)) {
                TERR("ProfileMultiValueIterator error: get multi-value over number limitation [%u : %u]!", ++_pos, _num);
                return NULL;
            }
            #endif
            char     * res = _targetAddr + 2;
            uint16_t *pLen = (uint16_t *)_targetAddr;
            _targetAddr  = _targetAddr + *pLen + 3;
            ++_pos;
            return res;
        }

        /*
         * 获取字段值的个数
         * @return 字段值个数
         */
        inline uint32_t getValueNum()
        {
            return _num;
        }

        /**
         * 设置迭代器元数据
         */
        inline void setMetaData(uint32_t valueNum, char *valueAddr, bool compress = false)
        {
            _pos        = -1;
            _num        = valueNum;
            _targetAddr = valueAddr;
            _compress   = compress;

            // 把初始值保存下来
            _old_pos        = _pos;
            _old_num        = _num;
            _old_targetAddr = _targetAddr;
            _old_compress   = _compress;
        }


        /**
         * 重置迭代器中的指针， 用于下一次遍历迭代
         */
        inline void resetIter()
        {
            _pos        = _old_pos;
            _num        = _old_num;
            _targetAddr = _old_targetAddr;
            _compress   = _old_compress;

            memset( _buffer, 0, sizeof(_buffer) );
        }


        /**
         * 获取数据地址
         */
        inline char * getDataAddress()
        {
            return _targetAddr;
        }

    private:
        uint32_t             _num;                                     //字段值的个数
        int32_t              _pos;                                     //记录取字段值的位置下标（pos+1为下标值)，初始化为-1
        char*                _targetAddr;                              //遍历过程使用的空间地址
        char                 _buffer[MAX_ENCODE_VALUE_NUM * 8];        //用于保存varint解压缩数据的buffer
        ProfileVarintWrapper _wrapper;                                 //varint解压缩操作包装类对象
        bool                 _compress;                                //是否进行varint压缩

        // 把初始值保存下来
        uint32_t             _old_num;                                 //字段值的个数
        int32_t              _old_pos;                                 //记录取字段值的位置下标（pos+1为下标值)，初始化为-1
        char*                _old_targetAddr;                          //遍历过程使用的空间地址
        bool                 _old_compress;                            //是否进行varint压缩

};

}

#endif //KINGSO_INDEXLIB_PROFILEMULTIVALUEITERATOR_H_
