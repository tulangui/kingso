/** \file
 ********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 8177 $
 *
 * $LastChangedDate: 2011-11-25 15:04:03 +0800 (Fri, 25 Nov 2011) $
 *
 * $Id: CfgManager.h 8177 2011-11-25 07:04:03Z xiaoleng.nj $
 *
 * $Brief: search模块配置解析读取 $
 ********************************************************************
 */

#ifndef _CFG_MANAGER_H_
#define _CFG_MANAGER_H_

#include "util/HashMap.hpp"
#include "util/Vector.h"
#include "commdef/commdef.h"
#include "mxml.h"

#define MAX_ELEMENT_LEN 1024

namespace search{

typedef struct _fieldconfnode
{
    char                             analyzer[64];       // 分词器
    util::HashMap<const char*, bool> stop_words;         // 停用词表
    bool                             ignore_stop_word;
} FieldConfNode;

typedef struct _occ_interval_t
{
    uint32_t min;
    uint32_t max;
} OccInterval;

typedef struct _package_element_t
{
    char    field_name[MAX_FIELD_VALUE_LEN];
    int32_t occ_min;
    int32_t occ_max;
    int32_t weight;
} PackageElement;

typedef struct _package_t
{
    char           *name;
    PackageElement *elements[MAX_ELEMENT_LEN];
    int32_t         size;
}Package;

class CfgManager
{
public:

    CfgManager();
 
    ~CfgManager();

    /**
     * 解析字段配置信息
     * @param [in] config config文件的xml node
     * @return 0 正常 非0 错误码
     */
    int parse(mxml_node_t *config);
 
    /**
     * 获取字段名对应的分词器名
     * @param [in] field_name 字段名 
     * @return 分词器名
     */
    char* getAnalyzerName(const char* field_name);
 
    /**
     * 字段对应值是否是停用词
     * @param [in] field_name 字段名 
     * @param [in] field_value 字段值 
     * @return true 停用词 false 非停用词
     */
    bool isStopWord(const char* field_name, const char* field_value);
    
    /**
     * 停用词是否忽略
     * @param [in] field_name 字段名
     * @return true 忽略 false 整个query空结果
     */
    bool stopWordIgnore(const char* field_name);

    /**
     * 获取异常结果长度配置值
     * @return 异常长度
     */
    uint32_t getAbnormalLen();

    Package *getPackage(const char *package_name);
private:
    int addPackageElement(char *package_name, 
                          char *sub_field, 
                          char *p_occ, 
                          char *p_weight);
    
    util::HashMap<const char*, FieldConfNode*> _field_map;        // field配置信息
    UTIL::HashMap<const char*, Package*>       _packages;
    uint32_t                                   _abnormal_len;     // 异常查询结果长度
    
};

}


#endif
