#ifndef _PROFILE_READER_H_
#define _PROFILE_READER_H_

#include "index_lib/ProfileManager.h"
#include "index_lib/ProfileDocAccessor.h"
#include "index_lib/ProfileMultiValueIterator.h"

#include <stdint.h>
#include <map>

typedef std::map<int64_t, int32_t> Map;
namespace sort_util
{
class ProfileReader{
public:
    ProfileReader(index_lib::ProfileDocAccessor *pd);
    ~ProfileReader(){}
    int ReadData(uint32_t* docids,const index_lib::ProfileField* pf, int docnum, void* pdest);
    int getMlrWeight(uint32_t* docids,const index_lib::ProfileField* pf, int docnum, Map &map, void* pBuff);
    int getDistLevel(uint32_t* docids, const index_lib::ProfileField* pf, int docnum, Map &map, void* pBuff, int multi_val_num);
private:
    mutable index_lib::ProfileMultiValueIterator _iterator;
    index_lib::ProfileDocAccessor *_pProfDA;
};

}

#endif
