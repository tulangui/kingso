/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: LatLngManager.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: LatLng字段的编解码接口类 $
 ********************************************************************/

#ifndef INDEXLIB_LATLNG_MANAGER_H_
#define INDEXLIB_LATLNG_MANAGER_H_

#include <stdint.h>
#include "IndexLib.h"

namespace index_lib
{

union LATLNGID 
{
    struct 
    {
        float lat;
        float lng;
    }latlng;

    uint64_t lvalue;
};

class LatLngManager
{ 
public:
    /**
     * 获取单实例对象指针, 实现类的单例
     *
     * @return  NULL: error
     */
    static LatLngManager * getInstance();

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
     * @param  lat    编码值对应的邮政编码
     * @param  lng    编码值对应的邮费
     */
    inline void decode(uint64_t code, float &lat, float &lng)
    {
        LATLNGID mid_stu;
        mid_stu.lvalue = code;
        lat = mid_stu.latlng.lat;
        lng = mid_stu.latlng.lng;
    }


private:
    LatLngManager();
    ~LatLngManager();

private:
    static LatLngManager   *_instance;         // 类的惟一实例
};

}


#endif //INDEXLIB_LATLNG_MANAGER_H_
