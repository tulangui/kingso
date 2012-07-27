#ifndef SORT_RESULT_H_
#define SORT_RESULT_H_
#include <stdint.h>
#include <string.h>
#include "util/MemPool.h"
#include "commdef/commdef.h"
#include "queryparser/Param.h"
#include "sort/OPDefine.h"
#include "index_lib/ProfileManager.h"

class MemPool;
class Document;

namespace sort_util
{
    class SDM;
}

namespace sort_framework
{

class SortResult 
{
public:
    SortResult(MemPool *mempool);
    ~SortResult();
    uint32_t serialize(char *str, uint32_t len) const;
    bool     deserialize(char *str, uint32_t len);
    int32_t getDocidList(uint32_t * & pDocid);
    int32_t      getNidList(int64_t *&pnid);
    int32_t      getSidList(int32_t *&snid);
    int32_t      getFirstRankList(int64_t *&pfirst);
    int32_t      getSecondRankList(int64_t *&psecond);
    int32_t      getPosList(int32_t * & pos);
    uint32_t getDocsFound();
    uint32_t getEstimateDocsFound();
	void  setSDM(sort_util::SDM *pSDM) {_sdm = pSDM;}
/*----------- for cache---------*/
public:
    int32_t FilterInvalidID(uint32_t nNum=0);
    uint32_t deserialize(char *str, uint32_t len, int32_t topN);
    uint32_t getBlockCount();
    void setBlockCount(uint32_t num);

public: 
    MemPool *_mempool; 
    sort_util::SDM *_sdm;
};
}
#endif //SORT_RESULT_H_
