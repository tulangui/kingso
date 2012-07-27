/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: RealPostFeeManager.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: real_post_fee字段的编解码接口类 $
 ********************************************************************/

#ifndef INDEXLIB_REALPOSTFEE_MANAGER_H_
#define INDEXLIB_REALPOSTFEE_MANAGER_H_

#include <stdint.h>
#include "IndexLib.h"

namespace index_lib
{

const char FEE_DELIM   = ':'; 
const char POST_DELIM  = '_';
const char VALUE_DELIM = ';';

#define RPF_MAX_VALUE_LEN 512

union RealPostFeeID 
{
    struct 
    {
        uint32_t post;
        float    fee;
    }rpf_stu;

    uint64_t rpf_id;
};

class RealPostFeeManager
{ 
public:
    /**
     * 获取单实例对象指针, 实现类的单例
     *
     * @return  NULL: error
     */
    static RealPostFeeManager * getInstance();

    /**
     * 释放类的单例
     */
    static void freeInstance();

    /**
     * 对原始字符串进行重新编码
     * @param  str     原始字符串
     * @param  destArr 输出数组起始地址
     * @param  size    destArr对应的输出数组大小
     * @return         分拆字段值个数; -1, ERROR; -2, partial Ok(size is too small)
     */
    int encode(const char* str, uint64_t *destArr, uint32_t size);

    /**
     * 根据编码值获取原始值
     * @param  code   编码值
     * @param  post   编码值对应的邮政编码
     * @param  fee    编码值对应的邮费
     */
    inline void decode(uint64_t code, uint32_t &post, float &fee)
    {
        RealPostFeeID mid_stu;
        mid_stu.rpf_id = code;
        post = mid_stu.rpf_stu.post;
        fee  = mid_stu.rpf_stu.fee;
    }


private:
    RealPostFeeManager();
    ~RealPostFeeManager();

private:
    static RealPostFeeManager   *_instance;         // 类的惟一实例
};

}


#endif //INDEX_LIB_REALPOSTFEE_MANAGER_H_
