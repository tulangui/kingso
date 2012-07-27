#include "sort/MergerSortFacade.h"
#include "SortConfig.h"
#include "SortQuery.h"
#include "AppManage.h"

namespace sort_framework {

    MergerSortFacade::MergerSortFacade()
        : _pConfig(NULL),
        _pAppManage(NULL)
    {
    }

    MergerSortFacade::~MergerSortFacade()
    {
        if (_pConfig)
            delete _pConfig;
        if (_pAppManage)
            delete _pAppManage;
    }

/**
 * 初始化 : 事先加载所有的排序类
 * @prarm  szSortCfgFile, 配置文件路径 
 * @return  0, succeed 
 * @return  else, failed 
 */
int MergerSortFacade::init(const char * szSortCfgFile)
{
    _pConfig = new SortConfig();
    if (!_pConfig)
        goto failed;
    if (_pConfig->parse(szSortCfgFile)){
        TLOG("failed to SortConfig Init()");
        goto failed;
    }
    _pAppManage = new AppManage();
    if (!_pAppManage)
        goto failed;
    if (_pAppManage->Init(*_pConfig)){
        TLOG("failed to AppManage Init()");
        goto failed;
    }
    return 0;
failed:
    if (_pConfig)
        delete _pConfig;
    if (_pAppManage)
        delete _pAppManage;
    return -1;
}

/** 
 * 排序模块的入口，驱动整个sort模块流程
 * @param context,
 * @param param, queryparser对query的解析数据
 * @praram ppcr, 多列searcher返回的结果
 * @praram arrSize, 返回结果的列数
 * @param ret, sort之后得到的结果
 * @return  0, succeed
 * @return  else, failed
 */
int32_t MergerSortFacade::doProcess(const framework::Context & context,
        const queryparser::Param & param,
        commdef::ClusterResult **ppcr, uint32_t arrSize, 
        sort_framework::SortResult &ret) 
{
    MemPool *mempool = context.getMemPool();
    if (!mempool) {
        return -1;
    }
    SortQuery *pQuery = NEW(mempool, SortQuery)(param);
    SDM *pSDM = NULL;
    pSDM = _pAppManage->doProcessOnMerger(*_pConfig, pQuery, ppcr, arrSize, mempool);
    if (pSDM == NULL) {
        return -1;
    }
    ret.setSDM(pSDM);
    return 0;
}

const char *MergerSortFacade::getBranchAttr(const queryparser::Param & param) const
{
    return  NULL;
}

}
