/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: LatLngProcessor.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: 根据客户端经纬度信息计算宝贝中最短距离 $
 ********************************************************************/

#ifndef INDEXLIB_LATLNG_PROCESSOR_H_
#define INDEXLIB_LATLNG_PROCESSOR_H_

#include <math.h>

#include "IndexLib.h"
#include "ProfileManager.h"
#include "LatLngManager.h"

#define PI 3.1415926535898
#define rad(d) (d*PI/180.0)
#define EARTH_RADIUS  6378.137
 
namespace index_lib
{

class LatLngProcessor
{ 
    public:
        /**
         * 构造函数
         * @param  pField    latlng字段对应的指针
         */
        LatLngProcessor(const ProfileField *pField)
        {
            _pMgr         = LatLngManager::getInstance();
            _pField       = pField;
            _pDocAccessor = ProfileManager::getDocAccessor();
        }

        ~LatLngProcessor() {}

        /**
         * 根据目标经纬度信息计算距离(单位km)
         * @param   srcLat   源位置纬度
         * @param   srcLng   源位置经度
         * @param   dstLat   目标位置纬度
         * @param   dstLng   目标位置经度
         *
         * @return   经纬度距离(km)
         */
        float getDistance(float srcLat, float srcLng, float dstLat, float dstLng)
        {
            double latDis = rad(srcLat) - rad(dstLat);
            double lonDis = rad(srcLng) - rad(dstLng);
            double s = 2 * asin(sqrt(pow(sin(latDis/2), 2) + cos(rad(srcLat)) * cos(rad(dstLat)) * pow(sin(lonDis/2), 2))) * EARTH_RADIUS;
            s = round(s * 10000) / 10000;
            return s;
        }

        /**
         * 读取目标doc对应位置中的最短距离
         * @param  lat       客户端位置纬度信息
         * @param  lng       客户端位置经度信息
         * @param  docid     目标docid
         * @return           最短距离(如果没有位置信息则返回最大值INVALID_DOUBLE)
         */
        inline double getMinDistance(float lat, float lng, uint32_t docid)
        {
            
            _pDocAccessor->getRepeatedValue(docid, _pField, _iterator);
            uint32_t   num   = _iterator.getValueNum();

            float dstlat = 0.0f;
            float dstlng = 0.0f;
            double dist   = 0.0f;
            double minDist = INVALID_DOUBLE;

            for(uint32_t pos = 0; pos < num; ++pos) {
                _pMgr->decode(_iterator.nextUInt64(), dstlat, dstlng);
                dist = getDistance(dstlat, dstlng, lat, lng);
                if (minDist > dist) {
                    minDist = dist;
                }
            }
            return minDist;
        }


        void printLatLng(uint32_t docid)
        {
            _pDocAccessor->getRepeatedValue(docid, _pField, _iterator);
            uint32_t   num   = _iterator.getValueNum();
            float lat = 0.0f;
            float lng = 0.0f;

            for(uint32_t pos = 0; pos < num; pos++) {
                _pMgr->decode(_iterator.nextUInt64(), lat, lng);
                printf("value%u:\t%f:%f\n",pos, lat, lng);
            }
        }

    private:
        index_lib::LatLngManager             *_pMgr;           // 编解码操作类的对象封装
        const index_lib::ProfileField        *_pField;         // latlng字段对应的ProfileField结构体指针
        index_lib::ProfileMultiValueIterator _iterator;        // 读取多值字段的迭代器对象
        index_lib::ProfileDocAccessor        *_pDocAccessor;   // 读取Profile所需的ProfileDocAccessor对象指针
};

}
#endif //INDEXLIB_LATLNG_PROCESSOR_H_
