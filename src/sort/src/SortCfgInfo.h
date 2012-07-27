#ifndef __SORT_CFGINFO_H_
#define __SORT_CFGINFO_H_
#include <vector>
#include <string>
#include <mxml.h>

namespace sort_framework
{
    //排序方式
    enum OrderType{
        ASC,    //升序
        DESC     //降序
    };

    //排序字段信息
    typedef struct _CmpFieldInfo
    {
        const char *Field;      //排序字段名称
        OrderType ord;          //指定排序方式
        const char *QueryParam; //可替换Field的query参数名称
    } CmpFieldInfo;

    //排序方法信息
    typedef struct _CompareInfo
    {
        const char *Name;   //名称
        uint32_t FieldNum;  //用于排序字段个数
        std::vector<CmpFieldInfo*> CmpFields;    //排序字段信息 
    } CompareInfo;

    //应用条件
    typedef struct _ConditionInfo
    {
        const char *Type;   //条件类型
        const char *Name;   //名称
        const char *Value;  //提交值
    } ConditionInfo;

    //应用信息
    typedef struct _ApplicationInfo
    {
        const char *Name; //
        const char *Type;   //条件类型
        bool isFinal;
        bool isTruncate;
        uint32_t CondNum;
        std::vector<ConditionInfo*> CondInfos;
        CompareInfo* pCmpInfo;
        mxml_node_t* xNode;
    } ApplicationInfo;

    //地址排序
    typedef struct _CustomInfo
    {
        ConditionInfo* pCondInfo;
        uint32_t AppNum;
        std::vector<ApplicationInfo*> AppInfos;
        std::string BrancheSpec;
        bool isTruncate;
    } CustomInfo;

}
#endif
