#include "Util/Reader/SDMReader.h"

using namespace index_lib;
namespace sort_util
{
    SDMReader::SDMReader(MemPool *mempool)
    {
        _mempool = mempool;
        _pProfManage = ProfileManager::getInstance();        
        _pProfDA = _pProfManage->getDocAccessor();
        _pProfileReader = NEW(_mempool, ProfileReader)(_pProfDA);
    }

    int SDMReader::getFieldInfo(const char* fieldName, Field_Info & fieldInfo)
    {
        if (!fieldName)
            return -1;
        const ProfileField * pf = _pProfDA->getProfileField(fieldName);
        if (!pf)
            return -1;
        fieldInfo.profField = pf;
        PF_FIELD_FLAG flag = _pProfDA->getFieldFlag(pf);
        fieldInfo.flag = flag;
        fieldInfo.MultiValNum = _pProfDA->getFieldMultiValNum(pf);
        fieldInfo.lenSize = 0;
        if (fieldInfo.MultiValNum == 1){    //单值
            fieldInfo.type = PT_PROFILE;
            switch (pf->getAppType()){
                case DT_INT8:
                case DT_UINT8: {
                        fieldInfo.lenSize = 1; 
                        break;
                    }
                case DT_INT16:
                case DT_UINT16: {
                        fieldInfo.lenSize = 2; 
                        break;
                    }
                case DT_INT32:
                case DT_UINT32: 
                case DT_FLOAT: {
                        fieldInfo.lenSize = 4; 
                        break;
                    }
                case DT_INT64:
                case DT_UINT64:
                case DT_DOUBLE:{
                        fieldInfo.lenSize = 8; 
                        break;
                    }
                default:{   //不支持其它类型
                        return -1;
                }
            }
        }
        else {
            TWARN("Do not support multi number profile field, field name is %s", fieldName);
            return -1;
        }
        return 0;
    }

    int SDMReader::ReadData(Field_Info& fieldInfo, uint32_t* docids, int docnum, void* pdest)
    {
        if ((!docids) || (!pdest) || (docnum<=0)){
            return -1;
        }
        switch (fieldInfo.type){
            case PT_PROFILE:{
               return _pProfileReader->ReadData(docids, fieldInfo.profField, docnum, pdest);
               break;
            }
            default:{
               break;
            }
        }
        return 0;
    }
}
