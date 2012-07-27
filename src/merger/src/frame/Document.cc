#include "Document.h"


bool Field::init(const char *pName, const char *pValue)
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

Field *Document::getField(const char *pName) const
{   
	if (!pName) return NULL;
	FIELDMAP::const_iterator tmpIter = _fieldsMap.find(pName);
	if (tmpIter != _fieldsMap.end())
		return tmpIter->value;
	return NULL;
}   

void Document::dump() {
	for (int32_t i=0; i<_fieldCount; i++) {
		if (_ppField[i]) {
			printf("field[%d]: name=[%s], value=[%s]\n", i,
					_ppField[i]->getName(), _ppField[i]->getValue());
		}
	}
	return;
}

bool Document::addField(const char *pName, const char *pValue)
{
	if (_fieldCount >= _fieldSize) {
		return false;
	}
	Field *pFieldTmp = NEW(_pHeap, Field)(_pHeap);
	if (!pFieldTmp->init(pName, pValue)) {
		return false;
	}
	_ppField[_fieldCount] = pFieldTmp;
	_fieldsMap.insert(pFieldTmp->getName(), pFieldTmp);
	_fieldCount++;
	return true;
}

void Document::init(int32_t fieldSize)
{
	if (fieldSize <= 0) {
		return;
	}
	_ppField = (Field **)NEW_VEC(_pHeap, Field *, fieldSize);
	_fieldSize = fieldSize;
	return;
}
