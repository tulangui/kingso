#ifndef _SDM_READER_H_
#define _SDM_READER_H_

#include "index_lib/ProfileManager.h"
#include "index_lib/ProfileDocAccessor.h"
#include "index_lib/ProfileMultiValueIterator.h"
#include "util/MemPool.h"
#include "ProfileReader.h"
#include <map>
typedef map<int64_t, int32_t> Map;
namespace sort_util
{

enum ProfileType
{
    PT_PROFILE,     //正常单值profile
};

typedef struct _Field_Info{
    const index_lib::ProfileField* profField; //fieldInfo
    PF_FIELD_FLAG flag;
    int MultiValNum;
    int lenSize;
    ProfileType type;
}Field_Info;

class SDMReader{
public:
    SDMReader(MemPool *_mempool);
    ~SDMReader(){}
    int getFieldInfo(const char* fieldName, Field_Info & fieldInfo);
    int ReadData(Field_Info& fieldInfo, uint32_t* docids, int docnum, void* pdest);
private:
    MemPool *_mempool;
    index_lib::ProfileManager* _pProfManage;
    index_lib::ProfileDocAccessor *_pProfDA;
    ProfileReader* _pProfileReader;
};

}
#endif
