/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: IndexConfigParams.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INDEX_LIB_INDEXCONFIGPARAMS_H_
#define INDEX_LIB_INDEXCONFIGPARAMS_H_

namespace index_lib
{

class IndexConfigParams
{
private:
    IndexConfigParams();
    ~IndexConfigParams();

public:
    static IndexConfigParams * getInstance();
    void setMemLock();
    bool isMemLock();

private:
    static IndexConfigParams _obj;

private:
    bool _flag;
};

}

#endif //INDEX_LIB_INDEXCONFIGPARAMS_H_
