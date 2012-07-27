#include "Util/Reader/ProfileReader.h"

using namespace index_lib;
namespace sort_util
{
ProfileReader::ProfileReader(ProfileDocAccessor *pd) : _pProfDA(pd)
{
}

int ProfileReader::ReadData(uint32_t* docids,const ProfileField* pf, int docnum, void* pBuff)
{
    if ((!docids) || (!pBuff) ||(!pf) || (docnum<=0)){
        return -1;
    }
    int i = 0;
    
    switch (pf->getAppType()) {
        case DT_INT8:{
            int8_t*  pdest = (int8_t*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getInt8(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_UINT8: {
            uint8_t*  pdest = (uint8_t*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getUInt8(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_INT16: {
            int16_t*  pdest = (int16_t*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getInt16(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_UINT16: {
            uint16_t*  pdest =(uint16_t*) pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getUInt16(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_INT32: {
            int32_t*  pdest = (int32_t*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getInt32(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_UINT32: {
            uint32_t*  pdest = (uint32_t*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getUInt32(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_INT64: {
            int64_t*  pdest = (int64_t*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getInt64(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_UINT64: {
            uint64_t*  pdest = (uint64_t*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getUInt64(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_FLOAT: {
            float*  pdest = (float*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getFloat(docids[i], pf);
                pdest++;
            }
            break;
        }
        case DT_DOUBLE: {
            double*  pdest = (double*)pBuff;
            for (;i < docnum; i++){
                *pdest = _pProfDA->getDouble(docids[i], pf);
                pdest++;
            }
            break;
        }
    }
    return docnum;
}

int ProfileReader::getMlrWeight(uint32_t* docids,const index_lib::ProfileField* pf, int docnum, Map &map, void* pBuff)
{
    if ((!docids) || (!pBuff) ||(!pf) || (docnum<=0))
        return -1;
    Map::iterator it;
    int64_t data = -1;
    int64_t weight = 0;

    for (int i=0; i<docnum; i++) {
        _pProfDA->getRepeatedValue(docids[i], pf, _iterator);
        uint32_t cnt = _iterator.getValueNum();
        switch(pf->getAppType()) {
            case DT_INT32: {
                for(uint32_t i = 0; i < cnt; i++) {
                    data = _iterator.nextInt32();
                    it = map.find(data);
                    if (it != map.end()){
                        weight += it->second;
                    }
                }
                *(int32_t*)pBuff = weight;
                break;
            }
            case DT_UINT32: {
                for(uint32_t i = 0; i < cnt; i++) {
                    data = _iterator.nextUInt32();
                    it = map.find(data);
                    if (it != map.end()){
                        weight += it->second;
                    }
                }
                *(int32_t*)pBuff = weight;
                break;
            }
            case DT_INT64: {
                for(uint32_t i = 0; i < cnt; i++) {
                    data = _iterator.nextInt64();
                    it = map.find(data);
                    if (it != map.end()){
                        weight += it->second;
                    }
                }
                *(int32_t*)pBuff = weight;
                break;
            }
            case DT_UINT64: {
                for(uint32_t i = 0; i < cnt; i++) {
                    data = _iterator.nextUInt64();
                    it = map.find(data);
                    if (it != map.end()){
                        weight += it->second;
                    }
                }
                *(int32_t*)pBuff = weight;
                break;
            }
            case DT_KV32: {
                for(uint32_t i = 0; i < cnt; i++) {
                    KV32 kv_data;
                    kv_data = _iterator.nextKV32();
                    data = ConvFromKV32(kv_data);
                    it = map.find(data);
                    if (it != map.end()){
                        weight += it->second;
                    }
                }
                *(int32_t*)pBuff = weight;
                break;
            }
            default: {
                TNOTE("multi profile format does not support this field");
                return -1;
            } 

        }
    }
    return docnum;
}

int ProfileReader::getDistLevel(uint32_t* docids, const index_lib::ProfileField* pf, int docnum, Map &map, void* pBuff, int multi_val_num)
{
    if ((!docids) || (!pBuff) ||(!pf) || (docnum<=0)) {
        return -1;
    }
    int64_t data = -1;
    Map::iterator it;
    int64_t weight = 0;

    if(multi_val_num == 1) {
        switch (pf->getAppType()) {
            case DT_INT32:
                for (int32_t i=0; i < docnum; i++){
                    data = _pProfDA->getInt32(docids[i], pf);
                    it = map.find(data);
                    if (it != map.end()){
                        weight = it->second;
                        break;
                    }
                }
                break;
            case DT_INT64:
                for (int32_t i=0; i < docnum; i++){
                    data = _pProfDA->getInt64(docids[i], pf);
                    it = map.find(data);
                    if (it != map.end()){
                        weight = it->second;
                        break;
                    }
                }
                break;
            default : 
                break;
        }
    } else {

        for (int i=0; i<docnum; i++) {
            _pProfDA->getRepeatedValue(docids[i], pf, _iterator);
            uint32_t cnt = _iterator.getValueNum();
            switch(pf->getAppType()) {
                case DT_INT32: {
                    for(uint32_t i = 0; i < cnt; i++) {
                        data = _iterator.nextInt32();
                        it = map.find(data);
                        if (it != map.end()){
                            weight = it->second;
                            break;
                        }
                    }
                    break;
                }
                case DT_UINT32: {
                    for(uint32_t i = 0; i < cnt; i++) {
                        data = _iterator.nextUInt32();
                        it = map.find(data);
                        if (it != map.end()){
                            weight = it->second;
                            break;
                        }
                    }
                    break;
                }
                case DT_INT64: {
                    for(uint32_t i = 0; i < cnt; i++) {
                        data = _iterator.nextInt64();
                        it = map.find(data);
                        if (it != map.end()){
                            weight = it->second;
                            break;
                        }
                    }
                    break;
                }
                case DT_UINT64: {
                    for(uint32_t i = 0; i < cnt; i++) {
                        data = _iterator.nextUInt64();
                        it = map.find(data);
                        if (it != map.end()){
                            weight = it->second;
                            break;
                        }
                    }
                    break;
                }
                default: {
                    TNOTE("multi profile format does not support this field");
                    return -1;
                } 
            }
        }
    }
    *(int32_t*)pBuff = weight;
    return weight;
}

}
