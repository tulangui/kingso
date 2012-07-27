#include "index_lib/LatLngManager.h"

#define LATLNG_MAX_VALUE_LEN 512

namespace index_lib
{

LatLngManager * LatLngManager::_instance = NULL;

static const char LATLNG_DELIM = ':';
static const char VALUE_DELIM  = ' ';

/**
 * 构造及析构函数
 * @return
 */
LatLngManager::LatLngManager() {}
LatLngManager::~LatLngManager(){}

/**
 * 获取单实例对象指针, 实现类的单例
 *
 * @return  NULL: error
 */
LatLngManager * LatLngManager::getInstance()
{
    if (unlikely(NULL == _instance))
    {
        _instance = new LatLngManager();
    }
    return _instance;
}


/**
 * 释放类的单例
 */
void LatLngManager::freeInstance()
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
int LatLngManager::encode(const char* str, uint64_t *destArr, uint32_t size)
{
    if (unlikely(str == NULL || destArr == NULL)) {
        return -1;
    }
    
    LATLNGID mid_stu;
    char buf[LATLNG_MAX_VALUE_LEN];
    
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
        if (unlikely(len >= LATLNG_MAX_VALUE_LEN)) {
            TWARN("LatLngManager encode error, small middle buffer!");
            return -1;
        }
        snprintf(buf, len + 1, "%s", begin);
        pEnd = rindex(buf, LATLNG_DELIM);
        if (unlikely(pEnd == NULL)) {
            TWARN("LatLngManager encode error, [%s] can not find delim[%c]!", buf, LATLNG_DELIM);
            return -1;
        }

        mid_stu.latlng.lng = strtof(pEnd + 1, NULL);
        mid_stu.latlng.lat = strtof(buf, NULL);

        if (unlikely(mid_stu.latlng.lat > 90 || mid_stu.latlng.lat < -90)) {
            TWARN("LatLngManager [%s] encode error, lat value [%f] over valid range[-90 ~ 90]!", buf, mid_stu.latlng.lat);
            return -1;
        }
        if (unlikely(mid_stu.latlng.lng > 180 || mid_stu.latlng.lng < -180)) {
            TWARN("LatLngManager [%s] encode error, lng value [%f] over valid range[-180 ~ 180]!", buf, mid_stu.latlng.lng);
            return -1;
        }

        if (unlikely(num >= (int)size)) {
            TWARN("LatLngManager encode error, output array is small!");
            return -2;
        }
        destArr[num++] = mid_stu.lvalue;
        begin = end + 1;
    } // while

    if (*begin != '\0') {
        snprintf(buf, LATLNG_MAX_VALUE_LEN, "%s", begin);
        pEnd = rindex(buf, LATLNG_DELIM);
        if (unlikely(pEnd == NULL)) {
            TWARN("LatLngManager encode error, [%s] can not find delim[%c]!", buf, LATLNG_DELIM);
            return -1;
        }

        mid_stu.latlng.lng = strtof(pEnd + 1, NULL);
        mid_stu.latlng.lat = strtof(buf, NULL);

        if (unlikely(mid_stu.latlng.lat > 90 || mid_stu.latlng.lat < -90)) {
            TWARN("LatLngManager [%s] encode error, lat value [%f] over valid range[-90 ~ 90]!", buf, mid_stu.latlng.lat);
            return -1;
        }
        if (unlikely(mid_stu.latlng.lng > 180 || mid_stu.latlng.lng < -180)) {
            TWARN("LatLngManager [%s] encode error, lng value [%f] over valid range[-180 ~ 180]!", buf, mid_stu.latlng.lng);
            return -1;
        }
        if (unlikely(num >= (int)size)) {
            TWARN("LatLngManager encode error, output array is small!");
            return -2;
        }
        destArr[num++] = mid_stu.lvalue;
    }
    return num;
}


}
