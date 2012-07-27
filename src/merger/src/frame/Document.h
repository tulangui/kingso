#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include "util/MemPool.h"
#include "util/HashMap.hpp"
#include "util/namespace.h"
#include <map>
#include <string>

class Field;
typedef util::HashMap<const char*, Field *> FIELDMAP;

class Field
{
    public:
        Field(MemPool *pHeap) 
            :_pName(NULL), 
            _pValue(NULL), 
            _pHeap(pHeap) {;}
        bool init(const char *pName, const char *pValue);
        char *getValue() const {return _pValue;}
        char *getName() const {return _pName;}
    private:
        char *_pName;
        char *_pValue;
        MemPool *_pHeap;
};

class Document
{
    public:
        Document(MemPool *pHeap)
            :_ppField(NULL), 
            _fieldCount(0), 
            _fieldSize(0),
            _pHeap(pHeap) {;}
        void init(int32_t fieldSize);
        int32_t getFieldCount() const {return _fieldCount;}
        bool addField(const char *pName, const char *pValue);
        Field *getField(int32_t pos) const {return _ppField[pos];}
        Field *getField(const char *pName) const;
        void dump();
    private:
        Field **_ppField;
        int32_t _fieldCount;
        int32_t _fieldSize;
        FIELDMAP _fieldsMap;
        MemPool *_pHeap;
};

#endif
