/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: RealPostFeeProcessor.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INDEX_LIB_REALPOSTFEEPROCESSOR_H_
#define INDEX_LIB_REALPOSTFEEPROCESSOR_H_

#include "IndexLib.h"
#include "ProfileManager.h"
#include "RealPostFeeManager.h"

namespace index_lib
{

class RealPostFeeProcessor
{ 
    public:
        /**
         * 构造函数
         * @param  pField    real_post_fee字段对应的指针
         */
        RealPostFeeProcessor(const ProfileField *pField)
        {
            _pMgr         = RealPostFeeManager::getInstance();
            _pField       = pField;
            _pDocAccessor = ProfileManager::getDocAccessor();
        }

        ~RealPostFeeProcessor() {}

        /**
         * 读取目标doc对应的目标地区的精确邮费值
         * @param  usrloc    目标地区邮编值
         * @param  docid     目标docid
         * @return           对应的精确邮费值
         */
        inline float getRealPostFee(int32_t usrloc, uint32_t docid)
        {
            if (usrloc == -1) {
                return 0;
            }
            _pDocAccessor->getRepeatedValue(docid, _pField, _iterator);
            uint32_t   num   = _iterator.getValueNum();
            uint64_t * array = (uint64_t *)_iterator.getDataAddress();

            float default_price = 0.0f;
            uint32_t loc = 0;
            float    fee = 0.0f;

            if (num > 0) {
                _pMgr->decode(array[0], loc, fee);
                if (usrloc == (int)loc) {
                    return fee;
                }
                if (loc == 1) {
                    default_price = fee;
                }
            }

            int start = 0;
            int end   = num - 1;
            int mid   = 0;
            while ( start <= end ) { 
                mid = (start + end) / 2;
                _pMgr->decode(array[mid], loc, fee);
                if ( loc == (uint32_t)usrloc ) { 
                    return fee;
                }   

                if ( loc > (uint32_t)usrloc ) { 
                    end = mid - 1;
                }   
                else { 
                    start = mid + 1;
                }   
            }   
            return default_price;
        }


        void printRealPostFee(uint32_t docid)
        {
            _pDocAccessor->getRepeatedValue(docid, _pField, _iterator);
            uint32_t   num   = _iterator.getValueNum();
            uint32_t loc = 0;
            float    fee = 0.0f;

            for(uint32_t pos = 0; pos < num; pos++) {
                _pMgr->decode(_iterator.nextUInt64(), loc, fee);
                printf("value%u:\t%u:%f\n",pos, loc, fee);
            }
        }

    private:
        index_lib::RealPostFeeManager        *_pMgr;           // 精确邮费编解码操作类的对象封装
        const index_lib::ProfileField        *_pField;         // real_post_fee字段对应的ProfileField结构体指针
        index_lib::ProfileMultiValueIterator _iterator;        // 读取多值字段的迭代器对象
        index_lib::ProfileDocAccessor        *_pDocAccessor;   // 读取Profile所需的ProfileDocAccessor对象指针
};

}
#endif //INDEX_LIB_REALPOSTFEEPROCESSOR_H_
