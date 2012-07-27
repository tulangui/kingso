/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: SentenceFilterParser.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_LESSMOREFILTERPARSER_H_
#define SEARCHER_LESSMOREFILTERPARSER_H_

#include "FilterParseInterface.h"
namespace queryparser {
class LessMoreFilterParser : public FilterParseInterface
{ 
public:
    LessMoreFilterParser();
    ~LessMoreFilterParser();
    
    virtual int doFilterParse(MemPool *mempool, FilterList *list, char *name, int nlen, char *string, int slen);
};


}
#endif 
