/*********************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: IndexInfoManager.h 2577 2011-03-09 01:50:55Z xiaojie.lgx $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INDEX_LIB_INDEXINFOMANAGER_H_
#define INDEX_LIB_INDEXINFOMANAGER_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "IndexLib.h"

#define INDEX_GLOBAL_INFO   "index.ginfo"

namespace index_lib {

class IndexInfoManager
{
public:
    IndexInfoManager();
    ~IndexInfoManager();

    /**
     * 载入(保存)指定的路径下的倒排索引信息
     *
     * @param path 倒排索引信息文件路径(名称固定)
     *
     * @return < 0: error;  >= 0: success.
     */
    int load(const char* path);
    int save(const char* path);


    /**
     * 获取Doc数量信息
     *
     * @return < 0: error;  >= 0: Doc数量信息
     */
    int getDocNum()   {    return _maxDocNum;   }


    /**
     * 变量所有的字段信息
     *
     * @param pos    in/out 获取的字段序号，初始化为0，由函数内部修改
     * @param fieldName out 得到的字段名
     * @param maxOccNum out 对应字段的occ信息
     *
     * @return < 0: error;  >= 0: success.
     */
    int getNextField(uint32_t& pos, char *& fieldName, uint32_t& maxOccNum);


    /**
     * 新加入一个字段信息
     *
     * @param fieldName 倒排字段名称
     * @param maxOccNum 字段的occ数量
     *
     * @return < 0: error; == 0:添加的字段重复 > 0: 当前字段数量
     */
    int addField(const char * fieldName, int maxOccNum);

    /**
     * 设置Doc数量信息
     *
     * @param docNum doc数量信息
     *
     * @return < 0: error;  >= 0: success.
     */
    int setDocNum(int32_t docNum);


private:
    int32_t   _maxDocNum;                                  // 全量最大doc
    int32_t   _fieldNum;                                   // 倒排字段数量
    char      _fieldName[MAX_INDEX_FIELD_NUM][PATH_MAX];   // 倒排字段名称
    int32_t   _maxOccNum[MAX_INDEX_FIELD_NUM];             // 每个倒排字段最大occ数量
};


}

#endif // INDEX_LIB_INDEXINFOMANAGER_H_

