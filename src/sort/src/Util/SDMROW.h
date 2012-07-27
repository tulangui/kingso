#ifndef _SDM_ROW_H_
#define _SDM_ROW_H_
#include <string>
#include "util/MemPool.h"
#include "Util/Reader/SDMReader.h"
#include "commdef/commdef.h"
#include "index_lib/IndexLib.h"

namespace sort_util
{

    //数据来源方式
    enum ROW_TYPE
    {
        RT_PROFILE,    //来源profile
        RT_COPY,   //来源拷贝其它Row
        RT_VIRTUAL,  //虚拟，中间数据
        RT_OTHER    //其他数据，维度与SDM不一致(uniq,distinct)
    };

    class SDM_ROW
    {
        public:
            SDM_ROW(){}
            virtual ~SDM_ROW(){}
            virtual void * getBuff() = 0;
            virtual void setBuff(char *srcBuff) = 0;
            //virtual void setBuff(void *ptr) = 0; 
            virtual int Alloc(int docNum = 0, int lenSize = 0) = 0;
            virtual void Clear() = 0;
            virtual bool Load() = 0;
            virtual bool Read(int no) = 0;
            virtual int fullData(char* pSrcData) = 0;
            virtual int getLenSize() = 0;
            virtual void setLenSize(int lenSize) = 0;
            virtual int getDocNum() = 0;
            virtual void setDocNum(int docNum) = 0;
            virtual void setDocIds(uint32_t* pdocIds) = 0;
            virtual SDM_ROW* getCopy() = 0;
            virtual bool setCopy(SDM_ROW* pRow) = 0;
            virtual bool isLoad() = 0;
            virtual bool isAlloc() = 0;
            virtual bool isOk() = 0;
            virtual void Add(uint32_t no, int32_t value) = 0;
            virtual void Add(uint32_t no, SDM_ROW* pRow) = 0;
            virtual void Multi(uint32_t no, float value) = 0;
            virtual void Multi(uint32_t no, SDM_ROW* pRow) = 0;
            virtual void Set(uint32_t no, double value) = 0;
            virtual double Get(uint32_t no) = 0;
            virtual uint64_t getHash(uint32_t no, uint64_t & reHash)  = 0;
            virtual void* getValAddr(int no) = 0;
            virtual void Copy(uint32_t to, uint32_t from) = 0;
            virtual bool needSerialize() = 0;
            virtual PF_DATA_TYPE getDataType() = 0;
            virtual void setRowType(ROW_TYPE type) = 0;
            virtual ROW_TYPE getRowType() = 0;
            virtual void addDocNum(int docNum) = 0;
            virtual void * getCurAddr() = 0;
            virtual void addCurNum(int curNum) = 0;
            virtual void Dump(FILE *pFile) = 0;
            virtual bool isEmptyValue(uint32_t i) = 0;

        public:
            std::string rowName;
            ROW_TYPE rowType; //来源类型
            PF_DATA_TYPE dataType;
    };

    template <typename T>
        class SDM_ROW_BASE : public SDM_ROW
    {
        public:
            SDM_ROW_BASE(){
                _dataBuff = NULL;
                _lenSize = 0;   //存储长度
                _pCopy = NULL;
                _docNum = 0; 
                _isLoad = false;
                _isAlloc = false;
                _needSerialize = false;
                _curNum = 0;
                _pdocIds = NULL;
                _isOk = false;
            }
            virtual ~SDM_ROW_BASE(){}
        public:

            //获取数据起始地址
            void * getBuff() {return _dataBuff;}
            void setBuff(char *pSrcBuff) {_dataBuff = pSrcBuff;}
            //获取指定数据所在地址
            void * getValAddr(int no) {
                char* p = (char*)_dataBuff;
                return (void*)(&p[no*_lenSize]);
            }
            //获取当前地址
            void * getCurAddr() {
                char* p = (char*)_dataBuff;
                return (void*)(&p[_curNum*_lenSize]);
            }
            //分配内存
            virtual int Alloc(int docNum = 0, int lenSize = 0){
                if (docNum  > 0)
                    _docNum = docNum;
                if (lenSize > 0)
                    _lenSize = lenSize;
                if ((!_mempool) || (_docNum<=0) || (_lenSize<=0))
                    return -1;
                int datalen = (_docNum * _lenSize);
                _dataBuff = NEW_ARRAY(_mempool, char , datalen);
                _isAlloc = true;
                return datalen;
            }

            virtual void Clear() {
                int datalen = (_docNum * _lenSize);
                if ((_dataBuff!=NULL) && (datalen>0)) {
                    memset(_dataBuff, 0x0, datalen);
                }
                return;
            }

            virtual bool Load(){
                if (_isLoad)
                    return true;
                return false;
            }

            virtual bool Read(int no){
                return true;
            }

            virtual int fullData(char* pSrcData){
                int datalen = (_docNum * _lenSize);
                if (!_dataBuff){
                    datalen = Alloc();
                    if (datalen < 0){
                        return datalen;
                    }
                }
                memcpy(_dataBuff, pSrcData, datalen);
                _isLoad = true;
                if (_pCopy){
                    _pCopy->fullData(pSrcData);
                }
                return datalen;
            }

            inline __attribute__((always_inline)) void Multi(uint32_t no, float value){
                switch (dataType) {
                    case DT_UINT8:
                        {
                            ((uint8_t*)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_UINT16:
                        {
                            ((uint16_t*)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_UINT32:
                        {
                            ((uint32_t*)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_UINT64:
                        {
                            ((uint64_t*)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_INT8:
                        {
                            ((int8_t*)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_INT16:
                        {
                            ((int16_t*)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_INT32:
                        {
                            ((int32_t*)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_INT64:
                        {
                            ((int64_t*)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_FLOAT:
                        {
                            ((float *)_dataBuff)[no] *= value;
                            break;
                        }
                    case DT_DOUBLE:
                        {
                            ((double *)_dataBuff)[no] *= value;
                            break;
                        }
                    default:
                        break;
                }
                return;
            }

            void Multi(uint32_t no, SDM_ROW* pRow){
                //((T*)_dataBuff)[no] *= value;
            }

            inline __attribute__((always_inline)) void Add(uint32_t no, int32_t value){
                switch (dataType) {
                    case DT_UINT8:
                        {
                            ((uint8_t*)_dataBuff)[no] += (uint8_t)value;
                            break;
                        }
                    case DT_UINT16:
                        {
                            ((uint16_t*)_dataBuff)[no] += (uint16_t)value;
                            break;
                        }
                    case DT_UINT32:
                        {
                            ((uint32_t*)_dataBuff)[no] += (uint32_t)value;
                            break;
                        }

                    case DT_UINT64:
                        {
                            ((uint64_t*)_dataBuff)[no] += (uint64_t)value;
                            break;
                        }

                    case DT_INT8:
                        {
                            ((int8_t*)_dataBuff)[no] += (int8_t)value;
                            break;
                        }

                    case DT_INT16:
                        {
                            ((int16_t*)_dataBuff)[no] += (int16_t)value;
                            break;
                        }

                    case DT_INT32:
                        {
                            ((int32_t*)_dataBuff)[no] += (int32_t)value;
                            break;
                        }

                    case DT_INT64:
                        {
                            ((int64_t*)_dataBuff)[no] += (int64_t)value;
                            break;
                        }
                    case DT_FLOAT:
                        {
                            ((float *)_dataBuff)[no] += (float)value;
                            break;
                        }
                    case DT_DOUBLE:
                        {
                            ((double *)_dataBuff)[no] += (double)value;
                            break;
                        }
                    default:
                        break;
                }
                return;
            }

            void Add(uint32_t no, SDM_ROW* pRow){
                //        ((T*)_dataBuff)[no] += (T)value;
            }

            /*
               T Get(uint32_t no){
               return ((T*)_dataBuff)[no];
               }
               */

            inline __attribute__((always_inline)) double Get(uint32_t no) {
                switch (dataType) {
                    case DT_UINT8:
                        {
                            return (double)((uint8_t *)_dataBuff)[no];
                        }
                    case DT_UINT16:
                        {
                            return (double)((uint16_t *)_dataBuff)[no];
                            break;
                        }
                    case DT_UINT32:
                        {
                            return (double)((uint32_t *)_dataBuff)[no];
                            break;
                        }
                    case DT_UINT64:
                        {
                            return (double)((uint64_t *)_dataBuff)[no];
                            break;
                        }
                    case DT_INT8:
                        {
                            return (double)((int8_t *)_dataBuff)[no];
                            break;
                        }
                    case DT_INT16:
                        {
                            return (double)((int16_t *)_dataBuff)[no];
                            break;
                        }
                    case DT_INT32:
                        {
                            return (double)((int32_t *)_dataBuff)[no];
                            break;
                        }
                    case DT_INT64:
                        {
                            return (double)((int64_t *)_dataBuff)[no];
                            break;
                        }
                    case DT_FLOAT:
                        {
                            return (double)((float *)_dataBuff)[no];
                            break;
                        }
                    case DT_DOUBLE:
                        {
                            return (double)((double *)_dataBuff)[no];
                            break;
                        }
                    default:
                        break;
                }
                return 0.0;
            }

            inline __attribute__((always_inline)) void Set(uint32_t no, double value){
                switch (dataType) {
                    case DT_UINT8:
                        {
                            ((uint8_t*)_dataBuff)[no] = (uint8_t)value;
                            break;
                        }
                    case DT_UINT16:
                        {
                            ((uint16_t*)_dataBuff)[no] = (uint16_t)value;
                            break;
                        }
                    case DT_UINT32:
                        {
                            ((uint32_t*)_dataBuff)[no] = (uint32_t)value;
                            break;
                        }

                    case DT_UINT64:
                        {
                            ((uint64_t*)_dataBuff)[no] = (uint64_t)value;
                            break;
                        }

                    case DT_INT8:
                        {
                            ((int8_t*)_dataBuff)[no] = (int8_t)value;
                            break;
                        }

                    case DT_INT16:
                        {
                            ((int16_t*)_dataBuff)[no] = (int16_t)value;
                            break;
                        }

                    case DT_INT32:
                        {
                            ((int32_t*)_dataBuff)[no] = (int32_t)value;
                            break;
                        }

                    case DT_INT64:
                        {
                            ((int64_t*)_dataBuff)[no] = (int64_t)value;
                            break;
                        }
                    case DT_FLOAT:
                        {
                            ((float *)_dataBuff)[no] = (float)value;
                            break;
                        }
                    case DT_DOUBLE:
                        {
                            ((double *)_dataBuff)[no] = (double)value;
                            break;
                        }
                    default:
                        break;
                }
                return;
            }

            void Move(uint32_t to, uint32_t from){
                T tmpData = ((T*)_dataBuff)[from];
                ((T*)_dataBuff)[from] = ((T*)_dataBuff)[to];
                ((T*)_dataBuff)[to] = tmpData;
            }
            void Copy(uint32_t to, uint32_t from){
                ((T*)_dataBuff)[to] = ((T*)_dataBuff)[from];
            }

            inline __attribute__((always_inline)) uint64_t getHash(uint32_t no, uint64_t & reHash){ 
                switch (dataType) {
                    case DT_UINT8:
                        {
                            reHash = ((uint8_t*)_dataBuff)[no];
                            break;
                        }
                    case DT_UINT16:
                        {
                            reHash = ((uint16_t*)_dataBuff)[no];
                            break;
                        }
                    case DT_UINT32:
                        {
                            reHash = ((uint32_t*)_dataBuff)[no];
                            break;
                        }
                    case DT_UINT64:
                        {
                            reHash = ((uint64_t*)_dataBuff)[no];
                            break;
                        }
                    case DT_INT8:
                        {
                            reHash = ((int8_t*)_dataBuff)[no];
                            break;
                        }
                    case DT_INT16:
                        {
                            reHash = ((int16_t*)_dataBuff)[no];
                            break;
                        }
                    case DT_INT32:
                        {
                            reHash = ((int32_t*)_dataBuff)[no];
                            break;
                        }
                    case DT_INT64:
                        {
                            reHash = ((int64_t*)_dataBuff)[no];
                            break;
                        }
                    case DT_FLOAT:
                        {
                            double tmp = ((float*)_dataBuff)[no];
                            memcpy(&reHash, &tmp, sizeof(double));
                            break;
                        }
                    case DT_DOUBLE: 
                        {
                            double tmp = ((double*)_dataBuff)[no];
                            memcpy(&reHash, &tmp, sizeof(double));
                            break;
                        }
                    default:
                        reHash = 0;
                }
                return reHash;
            }

            int getLenSize(){return _lenSize;}
            void setLenSize(int lenSize){_lenSize = lenSize;}
            int getDocNum(){return _docNum;}
            void setDocNum(int docNum){_docNum = docNum;}
            void addDocNum(int docNum) {_docNum += docNum;}
            int getCurNum(){return _curNum;}
            void setCurNum(int curNum){_curNum = curNum;}
            void addCurNum(int curNum) {_curNum += curNum;}

            void setDocIds(uint32_t* pdocIds) { _pdocIds=pdocIds;}
            SDM_ROW* getCopy(){return _pCopy;}
            bool setCopy(SDM_ROW* pRow){ 
                if ((!_pCopy) && (pRow)){
                    _pCopy = pRow;
                }
                return false;
            }

            bool isLoad() { return _isLoad;}
            bool isAlloc() { return _isAlloc;}
            bool isOk(){ return _isOk; }
            bool needSerialize() {return _needSerialize;}
            void setNeedSeri(bool isNeed) {_needSerialize = isNeed;}
            void Dump(FILE *pFile)
            {
                FILE *pTmpFile = stderr;
                if (pFile != NULL) {
                    pTmpFile = pFile;
                }
                fprintf(pFile, "name=%s, rowtype=%d, datatype=%d, lensize=%d, resnum=%d\n",
                        rowName.c_str(), rowType, getDataType(),
                        getLenSize(), getDocNum());
                switch (dataType) {
                    case DT_UINT8:
                    case DT_UINT16:
                    case DT_UINT32:{
                                       for (uint32_t i=0; i<getDocNum(); i++) {
                                           fprintf(pFile, "[%d] = %u\n", i, ((T*)_dataBuff)[i]);
                                       }
                                       break;
                                   }
                    case DT_INT8:
                    case DT_INT16:
                    case DT_INT32:{
                                      for (uint32_t i=0; i<getDocNum(); i++) {
                                          fprintf(pFile, "[%d] = %d\n", i, ((T*)_dataBuff)[i]);
                                      }
                                      break;
                                  }
                    case DT_UINT64: {
                                        for (uint32_t i=0; i<getDocNum(); i++) {
                                            fprintf(pFile, "[%d] = %lu\n", i, ((T*)_dataBuff)[i]);
                                        }
                                        break;
                                    }
                    case DT_INT64:{
                                      for (uint32_t i=0; i<getDocNum(); i++) {
                                          fprintf(pFile, "[%d] = %ld\n", i, ((T*)_dataBuff)[i]);
                                      }
                                      break;
                                  }
                    case DT_FLOAT:
                    case DT_DOUBLE: {
                                        for (uint32_t i=0; i<getDocNum(); i++) {
                                            fprintf(pFile, "[%d] = %.2f\t%.6f\t%6f\n", i, ((T*)_dataBuff)[i], ((T*)_dataBuff)[i], Get(i));
                                        }
                                        break;
                                    }
                    case DT_STRING: {
                                        for (uint32_t i=0; i<getDocNum(); i++) {
                                            char *pTmp = (char *)((char **)_dataBuff)[i];
                                            if (pTmp != NULL)
                                                fprintf(pFile, "[%d] = %s\n", i, pTmp);
                                            else
                                                fprintf(pFile, "[%d] = NULL\n", i);
                                        }
                                        break;
                                    }
                    default:
                                    fprintf(pFile, "unknow type\n");
                }
                return;
            }

            PF_DATA_TYPE getDataType(){ return dataType; }
            void setRowType(ROW_TYPE type) {rowType = type;}
            ROW_TYPE getRowType() {return rowType;}
            bool isEmptyValue(uint32_t i)
            {
                switch (dataType) {
                    case DT_UINT8:
                        {
                            return ((((uint8_t *)_dataBuff)[i])==INVALID_UINT8);
                        }
                    case DT_UINT16:
                        {
                            return ((((uint16_t *)_dataBuff)[i])==INVALID_UINT16);
                        }
                    case DT_UINT32:
                        {
                            return ((((uint32_t *)_dataBuff)[i])==INVALID_UINT32);
                        }
                    case DT_UINT64:
                        {
                            return ((((uint64_t *)_dataBuff)[i])==INVALID_UINT64);
                        }
                    case DT_INT8:
                        {
                            return ((((int8_t *)_dataBuff)[i])==INVALID_INT8);
                        }
                    case DT_INT16:
                        {
                            return ((((int16_t *)_dataBuff)[i])==INVALID_INT16);
                        }
                    case DT_INT32:
                        {
                            return ((((int32_t *)_dataBuff)[i])==INVALID_INT32);
                        }
                    case DT_INT64:
                        {
                            return ((((int64_t *)_dataBuff)[i])==INVALID_INT64);
                        }
                    case DT_FLOAT:
                        {
                            return ((((float *)_dataBuff)[i])==INVALID_FLOAT);
                        }
                    case DT_DOUBLE:
                        {
                            return ((((double *)_dataBuff)[i])==INVALID_DOUBLE);
                        }
                    default:
                        {
                            return false;
                        }
                }
                return false;
            }
        protected: 
            MemPool *_mempool;
            void* _dataBuff;
            int _lenSize;   //存储长度
            SDM_ROW* _pCopy;
            int _docNum;	//分配元素个数
            int _curNum;	//当前元素个数
            bool _isLoad;
            bool _isAlloc;
            uint32_t* _pdocIds;
            bool _isOk;
            bool _needSerialize;
    }; 

    template <typename T>
        class VIR_SDM_ROW : public SDM_ROW_BASE<T>
    {
        public:
            VIR_SDM_ROW()  {}
            VIR_SDM_ROW(MemPool *mempool, const char* pName,PF_DATA_TYPE type ,int lenSize){
                SDM_ROW_BASE<T>::_mempool = mempool;
                SDM_ROW_BASE<T>::rowName = pName;
                SDM_ROW_BASE<T>::rowType = RT_VIRTUAL;
                SDM_ROW_BASE<T>::dataType = type;
                SDM_ROW_BASE<T>::_lenSize = lenSize;   //存储长度
            }

            virtual bool Read(int no){
                return true;
            }
            virtual bool isLoad() {
                return true;
            }
    };


    template <typename T>
        class OTH_SDM_ROW : public SDM_ROW_BASE<T>
    {
        public:
            OTH_SDM_ROW(){}
            OTH_SDM_ROW(MemPool *mempool, const char* pName,PF_DATA_TYPE type ,int lenSize){
                SDM_ROW_BASE<T>::_mempool = mempool;
                SDM_ROW_BASE<T>::rowName = pName;
                SDM_ROW_BASE<T>::rowType = RT_OTHER;
                SDM_ROW_BASE<T>::dataType = type;
                SDM_ROW_BASE<T>::_lenSize = lenSize;   //存储长度
            }
            virtual bool Read(int no){
                return true;
            }
    };

    template <typename T>
        class COPY_SDM_ROW : public VIR_SDM_ROW<T>
    {
        public:
            COPY_SDM_ROW(MemPool *mempool, SDM_ROW* pRow){
                if (!pRow)
                    return;
                VIR_SDM_ROW<T>::_mempool = mempool;
                VIR_SDM_ROW<T>::rowName = "copy_"+pRow->rowName;
                VIR_SDM_ROW<T>::rowType = RT_COPY;
                VIR_SDM_ROW<T>::dataType = pRow->dataType;
                VIR_SDM_ROW<T>::_lenSize = pRow->getLenSize();   //存储长度
                VIR_SDM_ROW<T>::_docNum = pRow->getDocNum();
            }
            virtual bool Read(int no){
                return true;
            }
    };

    template <typename T>
        class TMP_SDM_ROW : public VIR_SDM_ROW<T>
    {
        public:
            TMP_SDM_ROW(MemPool *mempool, SDM_ROW* pRow){
                if (!pRow)
                    return;
                VIR_SDM_ROW<T>::_mempool = mempool;
                VIR_SDM_ROW<T>::rowName = "tmp_"+pRow->rowName;
                VIR_SDM_ROW<T>::rowType = RT_COPY;
                VIR_SDM_ROW<T>::dataType = pRow->dataType;
                VIR_SDM_ROW<T>::_lenSize = pRow->getLenSize();   //存储长度
            }
            virtual bool Read(int no){
                return true;
            }
    };

    template <typename T>
        class PRF_SDM_ROW : public SDM_ROW_BASE<T>
    {
        public:
            PRF_SDM_ROW(){}
            PRF_SDM_ROW(MemPool *mempool, const char* pName, SDMReader* pReader){
                SDM_ROW_BASE<T>::_isOk = false;
                SDM_ROW_BASE<T>::_mempool = mempool;
                SDM_ROW_BASE<T>::rowName = pName;
                profName = pName;
                SDM_ROW_BASE<T>::rowType = RT_PROFILE;
                _pSDMReader = pReader;
                if ((_pSDMReader->getFieldInfo(pName, _FieldInfo))<0){  //无法找到
                    return;
                }
                SDM_ROW_BASE<T>::_lenSize = _FieldInfo.lenSize;   //存储长度
                SDM_ROW_BASE<T>::dataType = _FieldInfo.profField->getAppType();
                SDM_ROW_BASE<T>::_isOk = true;
            }

            virtual bool Load(){
                if (SDM_ROW_BASE<T>::_isLoad)
                    return true;
                if (!SDM_ROW_BASE<T>::_dataBuff){
                    if (SDM_ROW_BASE<T>::Alloc() < 0){
                        return false;
                    }
                }
                uint32_t* pDocIds = (uint32_t*)SDM_ROW_BASE<T>::_pdocIds;
                char* pBuff = (char*)SDM_ROW_BASE<T>::_dataBuff;
                for (int i=0; i<SDM_ROW_BASE<T>::_docNum; i++){
                    if (pDocIds[i] != INVALID_DOCID){
                        if (_pSDMReader->ReadData(_FieldInfo, &pDocIds[i] , 1, pBuff) < 0)
                            return false;
                    }
                    pBuff += SDM_ROW_BASE<T>::_lenSize;
                }
                SDM_ROW_BASE<T>::_isLoad = true;
                if (SDM_ROW_BASE<T>::_pCopy){
                    SDM_ROW_BASE<T>::_pCopy->fullData((char*)SDM_ROW_BASE<T>::_dataBuff);
                }
                return true;
            }

            virtual bool Read(int no){
                void *pRes =(char*)SDM_ROW_BASE<T>::_dataBuff + (SDM_ROW_BASE<T>::_lenSize * no);
                if (_pSDMReader->ReadData(_FieldInfo, &SDM_ROW_BASE<T>::_pdocIds[no] , 1, pRes) < 0)
                    return false;
                return true;
            }

            virtual ~PRF_SDM_ROW(){}
        private:
            std::string profName;  //来源,必须是profile
            Field_Info _FieldInfo;
            SDMReader* _pSDMReader;
    };

}
#endif
