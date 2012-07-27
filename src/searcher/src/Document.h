/*********************************************************************
 * $Author: santai.ww $
 *
 * $LastChangedBy: santai.ww $
 * 
 * $Revision: 8355 $
 *
 * $LastChangedDate: 2011-12-01 16:02:41 +0800 (Thu, 01 Dec 2011) $
 *
 * $Id: Document.h 8355 2011-12-01 08:02:41Z santai.ww $
 *
 * $Brief: Document类，用于searcher框架调用 $
 ********************************************************************/

#ifndef KINGSO_SEARCHER_DOCUMENT_H
#define KINGSO_SEARCHER_DOCUMENT_H

#include "util/namespace.h"
#include "util/MemPool.h"

class Field
{
    public:
        Field(MemPool *pHeap) 
            :_pName(NULL), 
            _pValue(NULL), 
            _pHeap(pHeap) {;}
        bool init(const char *pName, const char *pValue)
        {
            if (pName==NULL || pValue==NULL || _pHeap==NULL) {
                return false;
            }
            _pName = (char *) NEW_VEC(_pHeap, char, (strlen(pName)+1));
            if (_pName == NULL) {
                return false;
            }
            memcpy(_pName, pName, strlen(pName)+1);
            _pValue = (char *) NEW_VEC(_pHeap, char, (strlen(pValue)+1));
            if (_pValue == NULL) {
                return false;
            }
            memcpy(_pValue, pValue, strlen(pValue)+1);
            return true;
        }
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
        void init(int32_t fieldSize)
        {
            if (fieldSize <= 0) {
                return;
            }
            _ppField = (Field **)NEW_VEC(_pHeap, Field *, fieldSize);
            _fieldSize = fieldSize;
            return;
        }
        int32_t getFieldCount() const {return _fieldCount;}
        bool addField(const char *pName, const char *pValue)
        {
            if (_fieldCount >= _fieldSize) {
                return false;
            }
            Field *pFieldTmp = NEW(_pHeap, Field)(_pHeap);
            if (!pFieldTmp->init(pName, pValue)) {
                return false;
            }
            _ppField[_fieldCount] = pFieldTmp;
            _fieldCount++;
            return true;
        }
        Field *getField(int32_t pos) const {return _ppField[pos];}
        void dump() {
            for (int32_t i=0; i<_fieldCount; i++) {
                if (_ppField[i]) {
                    printf("field[%d]: name=[%s], value=[%s]\n", i, 
                            _ppField[i]->getName(), _ppField[i]->getValue());
                }
            }
            return;
        }
    private:
        Field **_ppField;
        int32_t _fieldCount;
        int32_t _fieldSize;
        MemPool *_pHeap;
};

#endif
