/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: KeyvalueFilterParser.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_KEYVALUEFILTERPARSER_H_
#define SEARCHER_KEYVALUEFILTERPARSER_H_

#include "FilterParseInterface.h"

namespace queryparser {
class KeyvalueFilterParser : public FilterParseInterface
{ 
public:
    KeyvalueFilterParser();
    ~KeyvalueFilterParser();

    virtual int doFilterParse(MemPool *mempool, FilterList *list, char *name, int nlen, char *string, int slen);

};

}
#endif //SEARCHER_KEYVALUEFILTERPARSER_H_
