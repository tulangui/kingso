#include "sort/SearchSortFacade.h"
#include "SortConfig.h"
#include "SortQuery.h"
#include "AppManage.h"
#include "sys/time.h"

namespace sort_framework {

    SearchSortFacade::SearchSortFacade()
        : _pConfig(NULL),
        _pAppManage(NULL)
    {
    }

    SearchSortFacade::~SearchSortFacade()
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
    int SearchSortFacade::init(const char * szSortCfgFile)
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
     * @param context, 框架提供的资源类
     * @param param, queryparser对query的解析数据
     * @praram sr, 检索结果
     * @param ret, sort之后得到的结果
     * @return 0, success
     * @return else, failed
     */
    int SearchSortFacade::doProcess( const framework::Context &context,
            const queryparser::Param &param,
            SearchResult &sr,
            sort_framework::SortResult & ret)
    {
        MemPool *mempool = context.getMemPool();
        if (!mempool) {
            return -1;
        }
        SortQuery query(param);
        SDM *pSDM = _pAppManage->doProcessOnSearcher(*_pConfig, &query, sr, mempool);
        if (pSDM == NULL) {
            return -1;
        }
        ret.setSDM(pSDM);
        return 0;
    }

    /**
     * 获取该排序链条的属性（目前只有截断属性）
     * @param param, queryparser对query的解析数据
     * @return string, 属性字符串
     * @return NULL, failed
     */
    const char *SearchSortFacade::getBranchAttr(const queryparser::Param & param) const
    {
        SortQuery query(param);
        return _pAppManage->getBranchAttr(_pConfig, &query);
    }
}
