#include "SortConfig.h"
#include "util/Log.h"
#include "dlmodule/DlModuleManager.h"
namespace sort_framework{

SortConfig::SortConfig()
{
    _pTopNode = NULL;
    //默认按docid排序字段，追加到最后
    _pDefCmpFiled = new CmpFieldInfo;
    _pDefCmpFiled->Field = "docid";
    _pDefCmpFiled->ord = DESC;    //默认DESC
    _pDefCmpFiled->QueryParam = NULL;
}

SortConfig::~SortConfig()
{
    uint32_t i,j;
    if (_pDefCmpFiled)
        delete _pDefCmpFiled;
    for (i=0; i<_CmpInfoList.size(); i++) {
        for (j=0; j<_CmpInfoList[i]->FieldNum-1; j++){
            delete _CmpInfoList[i]->CmpFields[j];
        }
        delete _CmpInfoList[i];
    }  
    for (i=0; i<_AppInfoList.size(); i++) {
        for (j=0; j<_AppInfoList[i]->CondNum; j++){
            delete _AppInfoList[i]->CondInfos[j];
        }
        delete _AppInfoList[i];
    }
    for (i=0; i<_CusInfoList.size(); i++) {
        delete _CusInfoList[i]->pCondInfo;
        delete _CusInfoList[i];
    }
    if(_pTopNode)
        mxmlDelete(_pTopNode);
}

//解析sort配置文件
int SortConfig::parse(const char* filename)
{
    FILE *fp = fopen(filename, "r");
    if(NULL == fp)
    {
        TLOG( "配置文件打开出错%s\n", filename);
        return -1 ;
    }; 
    _pTopNode= mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
    fclose(fp);
    if(_pTopNode==NULL)
    {
        TLOG( "配置加载出错\n", filename);
        return -1 ;
    };
    mxml_node_t *xnode;
    int ret = 0;
    //找到compare节点
    xnode = mxmlFindElement(_pTopNode, _pTopNode, XML_COMPARES_FLAG, NULL, NULL, MXML_DESCEND);
    if(NULL == xnode)
    {
        TLOG( "配置文件格式错误%s\n", filename);
        return -1;
    }
    if ((ret = parseCompare(xnode)))
        return ret;

    //找到application节点
    xnode = mxmlFindElement(_pTopNode, _pTopNode, XML_APPLICATIONS_FLAG, NULL, NULL, MXML_DESCEND);
    if(NULL == xnode)
    {
        TLOG( "配置文件格式错误%s\n", filename);
        return -1;
    }
    if ((ret = parseApplication(xnode)))
        return ret;
    
    //找到custom节点
    xnode = mxmlFindElement(_pTopNode, _pTopNode, XML_CUSTOMS_FLAG, NULL, NULL, MXML_DESCEND);
    if(NULL == xnode)
        return 0;
    if ((ret = parseCustom(xnode)))
        return ret;

    if(false==dlmodule_manager::CDlModuleManager::getModuleManager()->parse(filename))
      {
        TLOG("load CDlModuleManager file failed");
        //return -1;
      }
    return 0;
}

//解析关于Compare部分配置
int SortConfig::parseCompare(mxml_node_t* xnode)
{
    mxml_node_t *xcmp;
    mxml_node_t *xfield;

    for(xcmp = mxmlFindElement(xnode, xnode, XML_COMPARE_FLAG, NULL, NULL, MXML_DESCEND_FIRST);
        xcmp;
        xcmp = mxmlFindElement(xcmp, xnode, XML_COMPARE_FLAG, NULL, NULL, MXML_NO_DESCEND))  
    {
        const char * pszCompName = mxmlElementGetAttr(xcmp, "name");
        if (!pszCompName) {
            TLOG("Compare配置错误被忽略");
            continue;
        }
        CompareInfo *pTmpCmp = new CompareInfo;
        pTmpCmp->Name = pszCompName;
        pTmpCmp->FieldNum = 0;
        for(xfield= mxmlFindElement(xcmp, xcmp, "field", NULL, NULL, MXML_DESCEND_FIRST);
            xfield;
            xfield = mxmlFindElement(xfield, xcmp, "field", NULL, NULL, MXML_NO_DESCEND))
        {
            const char * pszCmpFldName = mxmlElementGetAttr(xfield, "name");
            if (!pszCmpFldName){
                TLOG("Compare %s Field配置错误被忽略", pszCompName);
                continue;
            }
            CmpFieldInfo *pTmpCmpFiled = new CmpFieldInfo;
            pTmpCmpFiled->Field = pszCmpFldName;
            const char * pszCmpFldOrd = mxmlElementGetAttr(xfield, "order");
            if (!pszCmpFldOrd) {
                TLOG("Compare %s Field %s 未配置排序方式,默认以ASC方式排序", pszCompName, pszCmpFldName);
                pTmpCmpFiled->ord = ASC;    //默认ASC
            }
            else{
                if (strcmp(pszCmpFldOrd, "DESCENDING") == 0){
                    pTmpCmpFiled->ord = DESC;
                }
                else if (strcmp(pszCmpFldOrd, "ASCENDING") == 0){
                    pTmpCmpFiled->ord = ASC;    
                }
                else{
                    TLOG("Compare %s Field %s 排序方式配置错误,", pszCompName, pszCmpFldName);
                    //TLOG("默认以ASC方式排序");
                    pTmpCmpFiled->ord = ASC;    //默认ASC
                }
            }

            const char * pszCmpFldQName = mxmlElementGetAttr(xfield, "queryparam");
            pTmpCmpFiled->QueryParam = pszCmpFldQName;

            pTmpCmp->CmpFields.push_back(pTmpCmpFiled);
            pTmpCmp->FieldNum++;
        }
        if (pTmpCmp->FieldNum>0){
            //追加默认docid排序到CmpInfo
            pTmpCmp->CmpFields.push_back(_pDefCmpFiled);
            pTmpCmp->FieldNum++;
            _CmpInfoList.push_back(pTmpCmp);
        }
    }
    if (_CmpInfoList.size() > 0)
        return 0; 
    //TLOG("SORT未成功配置compare");
    return -1; 
}

//解析关于sort部分配置
int SortConfig::parseApplication(mxml_node_t* xnode)
{
    mxml_node_t *xapp;
    mxml_node_t *xcond;
    for(xapp = mxmlFindElement(xnode, xnode, XML_APPLICATION_FLAG, NULL, NULL, MXML_DESCEND_FIRST);
        xapp;
        xapp = mxmlFindElement(xapp, xnode, XML_APPLICATION_FLAG, NULL, NULL, MXML_NO_DESCEND))
    {
        const char * pszAppName = mxmlElementGetAttr(xapp, "name");
        const char * pszAppType = mxmlElementGetAttr(xapp, "type");
        const char * pszAppCmp = mxmlElementGetAttr(xapp, "compare");
        const char * pszAppIsFinal = mxmlElementGetAttr(xapp, "final");
        const char * pszAppIsTruncate = mxmlElementGetAttr(xapp, "truncate");

        if ((!pszAppName) || (!pszAppType)) {
            //TLOG("Application配置错误被忽略");
            continue;
        }
        ApplicationInfo *pTmpApp= new ApplicationInfo;
        pTmpApp->xNode = xapp;
        pTmpApp->Name = pszAppName;
        pTmpApp->Type = pszAppType;
        pTmpApp->isFinal = false;
        pTmpApp->isTruncate = false;
        if (pszAppIsFinal && pszAppIsFinal[0]) {
            if (strncmp(pszAppIsFinal, "true", strlen("true")) == 0) {
                pTmpApp->isFinal = true;
            }
        }
        if (pszAppIsTruncate && pszAppIsTruncate[0]) {
            if (strncmp(pszAppIsTruncate, "true", strlen("true")) == 0) {
                pTmpApp->isTruncate = true;
            }
        }
        pTmpApp->CondNum = 0;
        CompareInfo* pTmpCmpInfo = NULL;
        pTmpCmpInfo = GetCmpInfo(pszAppCmp);
        pTmpApp->pCmpInfo = pTmpCmpInfo;

        for(xcond= mxmlFindElement(xapp, xapp, "condition", NULL, NULL, MXML_DESCEND_FIRST);
            xcond;
            xcond = mxmlFindElement(xcond, xapp, "condition" , NULL, NULL, MXML_NO_DESCEND)) {
            const char * psAppCondName = mxmlElementGetAttr(xcond, "name");
            if (!psAppCondName){
                //TLOG("Application %s Field配置错误被忽略", psAppCondName);
                continue;
            }
            ConditionInfo *pTmpCondInfo= new ConditionInfo;
            pTmpCondInfo->Name = psAppCondName;
            const char * pszAppCondVal = mxmlElementGetAttr(xcond, "value");
            pTmpCondInfo->Value = pszAppCondVal;
            const char * pszAppCondType = mxmlElementGetAttr(xcond, "type");
            pTmpCondInfo->Type = pszAppCondType;
            pTmpApp->CondNum++;
            pTmpApp->CondInfos.push_back(pTmpCondInfo);
        }
        if (pTmpApp->CondNum>0){
           _AppInfoList.push_back(pTmpApp); 
        }
    }
    if (_AppInfoList.size()>0)
       return 0; 
    //TLOG("SORT未成功配置application");
    return -1;
}

int SortConfig::parseCustom(mxml_node_t* xnode) 
{ 
	mxml_node_t *xcustom;
	for(xcustom = mxmlFindElement(xnode, xnode, XML_CUSTOM_FLAG, NULL, NULL, MXML_DESCEND_FIRST);
        xcustom;
        xcustom = mxmlFindElement(xcustom, xnode, XML_CUSTOM_FLAG, NULL, NULL, MXML_NO_DESCEND))
    {
 		const char * pszCusParam = mxmlElementGetAttr(xcustom, "param"); 
 		const char * pszCusValue = mxmlElementGetAttr(xcustom, "value");  
 		const char * pszCusSort = mxmlElementGetAttr(xcustom, "sort");  
        const char * pszCusAttr = mxmlElementGetAttr(xcustom, "attr");
 		if ((!pszCusParam ) || (!pszCusValue ) || (!pszCusSort )){
 			 //TLOG("SORT未成功配置application"); 
 			  continue;
 		} 
 		CustomInfo *pTmpCusInfo = new CustomInfo;
        //init CustomInfo
        pTmpCusInfo->pCondInfo = NULL;
        pTmpCusInfo->AppNum = 0;
        pTmpCusInfo->isTruncate = false;

        if ((pszCusAttr!=NULL) && (strcmp(pszCusAttr, "truncate")==0)) {
            pTmpCusInfo->isTruncate = true;
        }
        pTmpCusInfo->BrancheSpec = pszCusSort;
        //Set ConditionInfo
 		ConditionInfo *pTmpCondInfo = new ConditionInfo ;
 		pTmpCondInfo->Name = pszCusParam;
 		pTmpCondInfo->Value = pszCusValue;
        pTmpCusInfo->pCondInfo = pTmpCondInfo;
        //split sort and check 
        _CusInfoList.push_back(pTmpCusInfo);
    }
    return 0; 
}

//根据AppName获取ApplicationInfo
ApplicationInfo* SortConfig::GetAppInfo(const char* pAppName)
{
    ApplicationInfo* retApp = NULL;
    if (!pAppName)
        return NULL;
    for (uint32_t i=0; i<_AppInfoList.size(); i++) {
        if (strcmp(_AppInfoList[i]->Name, pAppName)){
            retApp = _AppInfoList[i];
            break;
        }
    }
    return retApp;
}
//根据CmpName获取CompareInfo对象
CompareInfo* SortConfig::GetCmpInfo(const char* pCmpName)
{
    CompareInfo* retCmp = NULL;
    if (!pCmpName)
        return NULL;
    for (uint32_t i=0; i<_CmpInfoList.size(); i++) {
        if (strcmp(_CmpInfoList[i]->Name, pCmpName) == 0){
            retCmp = _CmpInfoList[i];
            break;
        }
    }
    return retCmp;
}

void SortConfig::splitString(const char * p, std::vector<std::string> & out)
{
    const char *end;
    if(!p) return;
    while(p) {
        if(*p == '\0') break;
        end = strchr(p, ' ');
        if(!end) {
            std::string str(p);
            out.push_back(str);
            break;
        }
        if(end > p) {
            std::string str(p, end - p );
            out.push_back(str);
        }
        p = end + 1;
    }
}

}

