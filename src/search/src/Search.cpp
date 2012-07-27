#include "search/Search.h"

#include "AndExecutor.h"
#include "NotExecutor.h"
#include "OrExecutor.h"
#include "PhraseExecutor.h"
#include "PostExecutor.h"
#include "FilterFactory.h"
#include "FilterInterface.h"
#include "SearchDef.h"
#include "AttrFilter.h"
#include "CfgManager.h"
#include "OccPostExecutor.h"
#include "LatLngAttrFilter.h"

#include "commdef/commdef.h"
#include "util/PriorityQueue.h"

using namespace index_lib;
using namespace framework;
using namespace queryparser;
using namespace util;
using namespace sort_framework;

namespace search {

Search::Search()
{
    _profile_accessor    = NULL;
    _p_index_reader      = NULL;
    _p_index_inc_manager = NULL;
    _p_del_map           = NULL;
    _p_filter_factory     = NULL;
    _p_prov_city_manager = NULL;
    _p_config_manager    = new CfgManager();
}


Search::~Search()
{
    SAFE_DELETE(_p_filter_factory);
    SAFE_DELETE(_p_config_manager);
}

int Search::init(mxml_node_t *config)
{
    _profile_accessor    = ProfileManager::getDocAccessor();	//获取profile DocAccessor指针
    _p_index_reader      = IndexReader::getInstance();		    //获取index reader指针
    _p_index_inc_manager = IndexIncManager::getInstance();		//获取增量index reader指针
    _p_docid_manager     = DocIdManager::getInstance();         //获取DocIdManager指针
    _p_prov_city_manager = ProvCityManager::getInstance();      //获取行政区管理器指针

    if (!_profile_accessor   || !_p_index_reader || !_p_index_inc_manager 
        || !_p_docid_manager || !_p_prov_city_manager)
    {
        return KS_ENOENT;
    }
    _p_del_map = _p_docid_manager->getDeleteMap();
    if (!_p_del_map){
        return KS_ENOENT;
    }
    if (!_p_filter_factory){
        _p_filter_factory = new FilterFactory();
        if (!_p_filter_factory){
          return KS_ENOMEM;
        }
    }
    if (!_p_filter_factory->init(_profile_accessor, _p_prov_city_manager, 
                                _p_del_map))
    {
        return KS_SE_INITERR;
    } 

    mxml_node_t  * modules_node  = NULL;
    mxml_node_t  * search_node   = NULL;     // search配置节点

    modules_node = mxmlFindElement(config, config, "modules", 
                                   NULL,   NULL,    MXML_DESCEND);
    if ( modules_node == NULL || modules_node->parent != config ) {
        TERR("can't find modules's node");
        return KS_EFAILED;
    }
    search_node = mxmlFindElement(modules_node, modules_node, "search", 
                                  NULL,         NULL,          MXML_DESCEND);
    if (search_node != NULL && search_node->parent == modules_node ) {     
        return _p_config_manager->parse(search_node);
    }

    return KS_SUCCESS;
}

bool Search::checkSyntaxTruncate(const char *field, qp_syntax_node_t *root)
{
    if(!root) {
        return true;
    }
    if(root->left_child == NULL && root->right_child == NULL) {
        if(root->field_name && (0 == strcmp(root->field_name, field))) {
            return false;
        }
    } else {
        checkSyntaxTruncate(field, root->left_child);
        checkSyntaxTruncate(field, root->right_child);
    }
    return true;
}

bool Search::checkTruncate(const char *field, QPResult *p_qp_result)
{
    FilterItemType type;
    char       *field_name      = NULL;
    char       *discount_field  = NULL;
    bool        is_negative     = false;
    uint32_t    item_num        = 0;
    FilterItem *field_items     = NULL;
    FilterList *p_filter        = p_qp_result->getFilterList();
    qp_syntax_tree_t *qp_tree  = p_qp_result->getSyntaxTree();
    if(qp_tree) {
        if( false == checkSyntaxTruncate(field, qp_tree->root)) {
            return false;
        }
    }
    if(p_filter) {
        p_filter->first();
        while (p_filter->next(field_name, discount_field, field_items, 
                              item_num,   type,           is_negative) == 0)
        {
            if(field_name && (0 == strcmp(field_name, field))) {
                return false;
            }
        }
    }
    return true;
}

Executor* Search::getPostExecutor(const char *field_str, 
                                  const char *field_value, 
                                  bool       &is_ignore,  
                                  MemPool    *mempool, 
                                  IndexMode   index_mode)
{
    const char *field_name  = field_str;
    char       *sub_field   = NULL;
    bool        is_occ_post = false; 
    char tmp_field[MAX_FIELD_VALUE_LEN];
    char tmp_sub[MAX_FIELD_VALUE_LEN] = {0};
    snprintf(tmp_field, sizeof(tmp_field), "%s", field_str);
    sub_field = strstr(tmp_field, "::");
    if(sub_field)  {
        sub_field[0] = sub_field[1] = 0;
        field_name   = tmp_field;
        sub_field    += 2;
        is_occ_post  = true; 
        snprintf(tmp_sub, sizeof(tmp_sub), "%s", sub_field);
    }

    if((index_mode == IDX_MODE_ALL && !_p_index_reader->hasField(field_name)) 
            || (index_mode == IDX_MODE_ADD && !_p_index_inc_manager->hasField(field_name))) 
    {
        is_ignore = true;
        return NULL;
    }
    if(_p_config_manager->isStopWord(field_name, field_value)) {    // 是stopword
        if(_p_config_manager->stopWordIgnore(field_name)) {
            is_ignore = true; 
        } else {
            is_ignore = false;
        }
        return NULL;
    }
    IndexTerm *p_index_term = NULL;
    if (index_mode == IDX_MODE_ALL) { //全量
        p_index_term = _p_index_reader->getTerm(mempool, field_name, field_value);
    } else {
        p_index_term = _p_index_inc_manager->getTerm(mempool, field_name, field_value);
    }
    if (!p_index_term) {   
        is_ignore = true;
        return NULL;
    }
    //创建 Executor
    PostExecutor *p_post_executor = NULL;
    if(is_occ_post) {
        p_post_executor = NEW(mempool, OccPostExecutor); 
        if (!p_post_executor) {
            TERR("NEW Mem for Search::parseIndex p_occ_executor failed"); 
            is_ignore = false;
            return NULL;
        }
    } else {
        p_post_executor = NEW(mempool, PostExecutor); 
        if (!p_post_executor) {
            TERR("NEW Mem for Search::parseIndex p_post_executor failed"); 
            is_ignore = false;
            return NULL;
        }
    }
    p_post_executor->setIndexTerm(p_index_term, field_name, field_value);
    Package *p_package = _p_config_manager->getPackage(field_name);
    if(p_package) {
        OccWeightCalculator *p_calculator = NEW(mempool,   OccWeightCalculator)
            (p_package, tmp_sub);
        if(!p_calculator) {
            TERR("alloc mem for Search::parseIndex OccWeightCalculator failed"); 
            is_ignore = false;
            return NULL;
        }
        p_post_executor->setOccWeightCalculator(p_calculator);
        if(is_occ_post) {
            OccFilter *p_occ_filter = NEW(mempool, OccFilter)(p_package, sub_field);
            if(!p_occ_filter) {
                TERR("alloc mem for OccFilter failed!");
                is_ignore = false;
                return NULL;
            }
            ((OccPostExecutor*)p_post_executor)->setOccFilter(p_occ_filter);
        }

    } else {
        p_post_executor->setOccWeightCalculator(NULL);
    }
    is_ignore = true;
    return p_post_executor;
}

Executor* Search::copyQpTree(qp_syntax_node_t* root,      
                             bool            &is_ignore, 
                             MemPool         *mempool, 
                             IndexMode        index_mode)
{
    Executor         *p_executor     = NULL;
    Executor         *left_executor  = NULL;
    Executor         *right_executor = NULL;
    char             *field_name     = NULL;
    char             *field_value    = NULL;
    if(!root) 
    {
        is_ignore = true;
        return NULL;
    }
    if(root->left_child == NULL && root->right_child == NULL) {
        field_name  = root->field_name;
        field_value = root->field_value;
        p_executor  = getPostExecutor(field_name, field_value, 
                                      is_ignore,  mempool,    index_mode);
        return p_executor; 
    } else {
        NodeLogicType relation = root->relation;
        if(relation==LT_AND){
            p_executor = NEW(mempool, AndExecutor); 
        } else if(relation==LT_OR){
            p_executor = NEW(mempool, OrExecutor); 
        } else if(relation==LT_NOT){
            p_executor = NEW(mempool, NotExecutor); 
        }
        left_executor  = copyQpTree(root->left_child, is_ignore, mempool,  index_mode);
        if(is_ignore == false)
        {
            return NULL;
        }
        right_executor = copyQpTree(root->right_child, is_ignore, mempool,  index_mode);
        if(is_ignore == false)
        {
            return NULL;
        }
        if(left_executor != NULL && right_executor != NULL)  {          //全不为空，插入
            if(relation != LT_NOT) {
                p_executor->addExecutor(left_executor);
                p_executor->addExecutor(right_executor);
            } else {
                p_executor->addExecutor(left_executor);
                ((NotExecutor*)p_executor)->setNotExecutor(right_executor);
            }
        } else if(left_executor == NULL && right_executor == NULL) {    //全为空，该node无效
            return NULL;
        } else {                                                        //有一个为空
            if(relation == LT_AND || relation == LT_OR) {
                return left_executor == NULL ? right_executor : left_executor;
            } else { 
                return left_executor; 
            }
        }
        return p_executor; 
    }
    return NULL;
}

int Search::parseAttrFilter(Context  *p_context,      
                            QPResult *p_qp_result, 
                            Executor *p_root_executor, 
                            MemPool  *mempool)
{
    FilterList *p_filter = p_qp_result->getFilterList();
    if ((!p_filter) || (!p_root_executor)){
        return 0;
    }
    p_filter->first(); 
    FilterItemType   type;
    int              flag               = 0;
    uint32_t         item_num           = 0;
    bool             is_negative        = false;
    char            *field_name         = NULL;
    FilterItem      *field_items        = NULL;
    char            *discount_field     = NULL;
    FilterInterface *p_filter_interface = NULL;

    //读取filter Info
    while (p_filter->next(field_name, discount_field, field_items, 
                          item_num,   type,           is_negative) == 0)
    {
        if ((!field_name) || (!field_items) || (item_num == 0)){
            continue;
        }
        if (strcmp(field_name, "olu") == 0) {
            continue;
        }
        flag = _p_filter_factory->create(p_context,          DT_PROFILE, 
                                         p_filter_interface, field_name, is_negative);
        if ((flag != KS_SUCCESS) && (flag != KS_EINVAL)) {
            return flag;
        }
        if (!p_filter_interface){
            continue;
        }
        for (uint32_t i = 0; i < item_num; i++) {
            if(type == ESingleType || type == EStringType) { 
                ((AttrFilter*)p_filter_interface)->addFiltValue(field_items[i].min);
            } else if(type == ERangeType) {
                Range<char*> range(field_items[i].min,      field_items[i].max, 
                                   field_items[i].minEqual, field_items[i].maxEqual);
                ((AttrFilter*)p_filter_interface)->addFiltRange(range);
            }
        }

        if(((AttrFilter*)p_filter_interface)->isLatLngType()) {
            // 是经纬度类型字段(distance 参数映射为 latlng字段) 
            float client_lat = 0.0f;
            float client_lng = 0.0f;
            if (unlikely(!p_qp_result->getParam()->getLatLngParam(client_lat, client_lng)))
            {
                TWARN("Filter call queryparser getLatLngParam failed!");
                continue;
            }

            // 设置客户端经纬度信息
            if (unlikely(!((LatLngAttrFilter *)p_filter_interface)->setLatLng(client_lat, client_lng)))
            {
                continue;
            }
        }
        else if(((AttrFilter*)p_filter_interface)->isPromotionType()) {
            time_t et = time(NULL);
            const char* p_et = p_qp_result->getParam()->getParam("!et");
            if(p_et) {
                et = atoi(p_et);
            }
            ((AttrFilter*)p_filter_interface)->setDiscount(field_name, et);
        }
        else if(discount_field) {
            time_t et = time(NULL);
            const char* p_et = p_qp_result->getParam()->getParam("!et");
            if(p_et) {
                et = atoi(p_et);
            }
            ((AttrFilter*)p_filter_interface)->setDiscount(discount_field, et);
        }
        p_root_executor->addFilter(p_filter_interface);
    }
    return KS_SUCCESS;
}

int Search::parseDelMapFilter(Context  *p_context, 
                              Executor *p_root_executor)
{                
    int              flag               = 0;
    FilterInterface *p_filter_interface = NULL; 
    flag = _p_filter_factory->create(p_context, 
                                     DT_DELMAP, 
                                     p_filter_interface);
    if (flag != KS_SUCCESS){
        return flag;
    }
    p_root_executor->addFilter(p_filter_interface);
    return KS_SUCCESS;
}

int Search::parseFilter(Context  *p_context,      
                        QPResult *p_qp_result, 
                        Executor *p_root_executor, 
                        MemPool  *mempool)
{ 
    int         flag     = 0;
    OluStatType olu_type;
    FilterList *p_filter = p_qp_result->getFilterList();
    if (p_filter) {
        flag = parseAttrFilter(p_context, 
                               p_qp_result, 
                               p_root_executor, 
                               mempool);
        if (flag != KS_SUCCESS){
            return flag;
        }
    }
    flag = parseDelMapFilter(p_context, p_root_executor);
    if (flag != KS_SUCCESS){
        return flag;
    }
    return KS_SUCCESS;
}

int  Search::getAndNodeNum( qp_syntax_node_t *root , int &node_num)
{
    if(!root) {
        return 0;
    }
    if(root->left_child == NULL && root->right_child == NULL) {
        return 0;
    } else {
        NodeLogicType relation = root->relation;
        if(relation==LT_AND){
            node_num++;
        } else if(relation==LT_OR){
        } else if(relation==LT_NOT){
            node_num++;
        } else{
        }
        getAndNodeNum(root->left_child, node_num);
        getAndNodeNum(root->right_child, node_num);
    }

    return 0;
}


int Search::doTruncate(MemPool                  *mempool, 
               queryparser::QPResult            *p_qp_result, 
               sort_framework::SearchSortFacade *p_sort_facade, 
               Executor                         *p_root_executor)
{
    uint32_t    truncate_len = 0;
    const char *ps_field     = NULL; 
    int node_num = 0;
    const char *p_use_trunc  = p_qp_result->getParam()->getParam("usetrunc");
    const char *p_sort_attr  = p_sort_facade->getBranchAttr(*(p_qp_result->getParam()));
    qp_syntax_tree_t *qp_tree  = p_qp_result->getSyntaxTree();
    while(1) {
        if(p_use_trunc != NULL && strcmp(p_use_trunc, "no") == 0 ) {
            break;
        }
        if(p_sort_attr == NULL ) {
            break;
        }
        if(strcmp(p_sort_attr, "truncate") != 0 ) {
            break;
        }
        getAndNodeNum(qp_tree->root, node_num);
        if(node_num > 14) {
            break;
        }
        ps_field = p_qp_result->getParam()->getParam("ps");
        if (ps_field && checkTruncate(ps_field, p_qp_result)) {
            truncate_len = p_root_executor->getTermsCnt();
            p_root_executor->truncate(ps_field, 0, _p_index_reader, _profile_accessor, mempool);
        }
        ps_field = p_qp_result->getParam()->getParam("_ps");
        if (ps_field && checkTruncate(ps_field, p_qp_result)) {
            truncate_len = p_root_executor->getTermsCnt();
            p_root_executor->truncate(ps_field, 1, _p_index_reader, _profile_accessor, mempool);
        }
        break;
    }
    return truncate_len;
}

int Search::doQuery(framework::Context       *p_context, 
            queryparser::QPResult            *p_qp_result, 
            sort_framework::SearchSortFacade *p_sort_facade, 
            SearchResult                     *p_search_result, 
            IndexMode                         mode,
            uint32_t                          init_docid)
{
    if (!p_context || !p_qp_result || !p_search_result){
        TERR2MSG(p_context, "QPResult is null or search_result is null"); 
        return KS_EINVAL;
    }
    MemPool *mempool = p_context->getMemPool();
    if (!mempool){
        TERR2MSG(p_context, "MemPool 为NULL"); 
        return KS_EINVAL;
    }
    //初始化SearchResult
    p_search_result->nDocFound         = 0;
    p_search_result->nEstimateDocFound = 0;
    p_search_result->nDocSize          = 0;
    p_search_result->nDocIds           = NULL;
    p_search_result->nDocSearch        = _p_docid_manager->getDocCount();

    qp_syntax_tree_t *qp_tree = p_qp_result->getSyntaxTree();
    if (!qp_tree){
        TERR2MSG(p_context, "qp_result->syntax_tree is null");
        return KS_SE_QUERYERR;
    }
    //进行倒排查询
    Executor   *p_root_executor     = NULL;
    Executor   *p_add_root_executor = NULL;
    Executor   *p_opt_executor      = NULL;
    bool        is_ignore           = false; 
    int         flag                = 0;
    uint32_t    truncate_len        = 0;
    float       truncate_rate       = 0.0;
    if(mode == IDX_MODE_ALL || mode == IDX_MODE_FULL) {
        p_root_executor = copyQpTree(qp_tree->root, is_ignore, mempool, IDX_MODE_ALL);
        if(p_root_executor) {

            p_opt_executor = p_root_executor->getOptExecutor(NULL, p_context);
            if(p_opt_executor) {

                p_root_executor = p_opt_executor;
            }
            truncate_len = doTruncate(mempool, p_qp_result, p_sort_facade, p_root_executor);
        }
    }
    if(mode == IDX_MODE_ADD || mode == IDX_MODE_FULL) {
        p_add_root_executor = copyQpTree(qp_tree->root, is_ignore, mempool,  IDX_MODE_ADD);
        if(p_add_root_executor) {
            p_opt_executor = p_add_root_executor->getOptExecutor(NULL, p_context);
            if(p_opt_executor) {
                p_add_root_executor = p_opt_executor;
            }
        }
    }
    if (!p_root_executor && !p_add_root_executor){ 
        return KS_SUCCESS;
    }
    uint32_t terms_count = 0;
    if(p_root_executor) {
        flag = parseFilter(p_context, p_qp_result, p_root_executor, mempool);
        if (flag != KS_SUCCESS){ 
            return flag;
        }
        terms_count = p_root_executor->getTermsCnt(); //预估最大结果数
        if(truncate_len > 0) {
            truncate_rate = (float)truncate_len / (float)terms_count ;
        }
    }
    if(p_add_root_executor) {
        flag = parseFilter(p_context, p_qp_result, p_add_root_executor, mempool);
        if (flag != KS_SUCCESS){ 
            return flag;
        }
        terms_count += p_add_root_executor->getTermsCnt(); //预估最大结果数
    }
    p_search_result->nDocIds = (uint32_t*)NEW_VEC(mempool, uint32_t, terms_count);
    p_search_result->nRank = (uint32_t*)NEW_VEC(mempool, uint32_t, terms_count);
    if ((!p_search_result->nDocIds) || (!p_search_result->nRank)){
        TERR2MSG(p_context, "内存分配失败 Search::doQuery ");
        return KS_ENOMEM;
    }
    uint32_t cut_len = _p_config_manager->getAbnormalLen();
    cut_len = (cut_len == 0 || cut_len > terms_count)? terms_count : cut_len;
    
    if(p_root_executor) {
        p_root_executor->doExecute(cut_len, 0, p_search_result);
        p_search_result->nDocFound = p_search_result->nDocSize;
        if(truncate_rate > 0.0) {
            p_search_result->nEstimateDocFound = p_search_result->nDocFound * truncate_rate;
        } else {
            p_search_result->nEstimateDocFound = p_search_result->nDocFound;
        }
    }
    if (p_add_root_executor) {
        p_add_root_executor->setPos(init_docid);
        p_add_root_executor->doExecute(cut_len, init_docid, p_search_result);
    }

    p_search_result->nEstimateDocFound += p_search_result->nDocSize - p_search_result->nDocFound;
    p_search_result->nDocFound = p_search_result->nDocSize;
    return KS_SUCCESS;
}

}
