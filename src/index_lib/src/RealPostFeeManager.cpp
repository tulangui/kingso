#include "index_lib/RealPostFeeManager.h"

namespace index_lib
{

RealPostFeeManager * RealPostFeeManager::_instance = NULL;

static int cmp_post_fee(const void * p1, const void * p2) 
{
    RealPostFeeID mid_stu1;
    RealPostFeeID mid_stu2;
    mid_stu1.rpf_id = *(uint64_t *)p1;
    mid_stu2.rpf_id = *(uint64_t *)p2;

    if (mid_stu1.rpf_stu.post > mid_stu2.rpf_stu.post) {
        return 1;
    }
    if (mid_stu1.rpf_stu.post < mid_stu2.rpf_stu.post) {
        return -1;
    }
    return 0;    
}

/**
 * 构造及析构函数
 * @return
 */
RealPostFeeManager::RealPostFeeManager() {}
RealPostFeeManager::~RealPostFeeManager(){}

/**
 * 获取单实例对象指针, 实现类的单例
 *
 * @return  NULL: error
 */
RealPostFeeManager * RealPostFeeManager::getInstance()
{
    if (unlikely(NULL == _instance))
    {
        _instance = new RealPostFeeManager();
    }
    return _instance;
}


/**
 * 释放类的单例
 */
void RealPostFeeManager::freeInstance()
{
    if (unlikely(_instance == NULL)) return;

    delete _instance;
    _instance = NULL;
}

/**
 * 对原始字符串进行重新编码
 * @param  str     原始字符串
 * @param  destArr 输出数组起始地址
 * @param  size    destArr对应的输出数组大小
 * @return         分拆字段值个数; -1, ERROR; -2, partial Ok(size is too small)
 */
int RealPostFeeManager::encode(const char* str, uint64_t *destArr, uint32_t size)
{
    if (unlikely(str == NULL || destArr == NULL)) {
        return -1;
    }
    
    RealPostFeeID mid_stu;
    char buf[RPF_MAX_VALUE_LEN];
    
    const char * begin = NULL;
    const char * end   = NULL;
    const char * pEnd  = NULL;
    int          num   = 0;
    uint32_t     len   = 0;

    begin = str;
    while((end = index(begin, VALUE_DELIM)) != NULL) {
        if (*begin == VALUE_DELIM) {
            begin = end + 1;
            continue;
        }   

        len = end - begin;
        if (unlikely(len >= RPF_MAX_VALUE_LEN)) {
            TWARN("RealPostFeeManager encode error, small middle buffer!");
            return -1;
        }
        snprintf(buf, len + 1, "%s", begin);

        pEnd = rindex(buf, FEE_DELIM);
        if (unlikely(pEnd == NULL)) {
            TWARN("RealPostFeeManager encode error, [%s] can not find fee part!", buf);
            return -1;
        }

        mid_stu.rpf_stu.fee = strtof(pEnd + 1, NULL);
        begin = buf;
        while((pEnd = index(begin, POST_DELIM)) != NULL) {
            mid_stu.rpf_stu.post = strtoul(begin, NULL, 10);
            if (unlikely(num >= (int)size)) {
                TWARN("RealPostFeeManager encode error, output array is small!");
                return -2;
            }
            destArr[num++] = mid_stu.rpf_id;
            begin = pEnd + 1;
        }   
        mid_stu.rpf_stu.post = strtoul(begin, NULL, 10);
        if (unlikely(num >= (int)size)) {
            TWARN("RealPostFeeManager encode error, output array is small!");
            return -2;
        }
        destArr[num++] = mid_stu.rpf_id;
        begin = end + 1;
    } // while

    if (*begin != '\0') {
        snprintf(buf, RPF_MAX_VALUE_LEN, "%s", begin);
        pEnd = rindex(buf, FEE_DELIM);
        if (unlikely(pEnd == NULL)) {
            TWARN("RealPostFeeManager encode error, [%s] can not find fee part!", buf);
            return -1;
        }

        mid_stu.rpf_stu.fee = strtof(pEnd + 1, NULL);
        begin = buf;
        while((pEnd = index(begin, POST_DELIM)) != NULL) {
            mid_stu.rpf_stu.post = strtoul(begin, NULL, 10);
            if (unlikely(num >= (int)size)) {
                TWARN("RealPostFeeManager encode error, output array is small!");
                return -2;
            }
            destArr[num++] = mid_stu.rpf_id;
            begin = pEnd + 1;
        }   
        mid_stu.rpf_stu.post = strtoul(begin, NULL, 10);
        if (unlikely(num >= (int)size)) {
            TWARN("RealPostFeeManager encode error, output array is small!");
            return -2;
        }
        destArr[num++] = mid_stu.rpf_id;
    }

    qsort(destArr, num, sizeof(uint64_t), cmp_post_fee);
    return num;
}


}
