#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ifaddrs.h>

#include "Config.h"
#include "util/Log.h"
#include "util/filefunc.h"
#include "util/iniparser.h"

namespace update {

Config::Config()
{
    memset(this, 0, sizeof(Config));
    _flux_limit = (uint64_t)-1;
    _cluster_thread_num = 16;
}

Config::~Config()
{

}

int Config::init(const char* cfgFile, ROLE role)
{
    ini_context_t ctx;
    if(ini_load_from_file(cfgFile, &ctx)){
        TERR("load config error, return!");
        return -1;
    }
    
    // global_info
    const char* path = ini_get_str_value("global_info", "cluster_conf", &ctx);
    if(!path || path[0]==0){
        TERR("get cluster_conf config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    snprintf(_cluster_conf, PATH_MAX, "%s", path);

    path = ini_get_str_value("global_info", "build_conf", &ctx);
    if(!path || path[0]==0){
        TERR("get build_conf config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    snprintf(_build_conf, PATH_MAX, "%s", path);
                          
    path = ini_get_str_value("global_info", "dispatcher_field_name", &ctx);
    if(!path || path[0]==0){
        TERR("get dispatcher_field_name config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    snprintf(_dispatcher_field, PATH_MAX, "%s", path);

    // enter_updater
    path = ini_get_str_value("enter_updater", "data_bak_path", &ctx);
    if(!path || path[0]==0){
        TERR("get enter_updater data_bak_path config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    snprintf(_enter_data_path, PATH_MAX, "%s", path);
    if(make_dir(_enter_data_path, S_IRWXU | S_IRWXG | S_IRWXO)) {
        TERR("mkdir %s error, return!", _enter_data_path);
        ini_destroy(&ctx);
        return -1;
    }
    
    path = ini_get_str_value("enter_updater", "message_sign_dict_path", &ctx);
    if(!path || path[0]==0){
        TERR("get enter_updater message_sign_dict_path config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    snprintf(_dotey_sign_path, PATH_MAX, "%s", path);
    if(make_dir(_dotey_sign_path, S_IRWXU | S_IRWXG | S_IRWXO)) {
        TERR("mkdir %s error, return!", _dotey_sign_path);
        ini_destroy(&ctx);
        return -1;
    }

    path = ini_get_str_value("enter_updater", "listening_port", &ctx);
    if(!path || path[0]==0){
        TERR("get enter_updater listening_port config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    _enter_port = atol(path);

    path = ini_get_str_value("enter_updater", "listening_port", &ctx);
    if(!path || path[0]==0){
        TERR("get enter_updater listening_port config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    _enter_send_timeout = atol(path);
   
    // cluster_updater
    path = ini_get_str_value("cluster_updater", "listening_port", &ctx);
    if(!path || path[0]==0){
        TERR("get cluster_updater listening_port config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    _cluster_port = atol(path);

    path = ini_get_str_value("cluster_updater", "send_timeout", &ctx);
    if(!path || path[0]==0){
        TERR("get cluster_updater send_timeout config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    _cluster_send_timeout = atol(path);

    path = ini_get_str_value("cluster_updater", "max_per_handles", &ctx);
    if(path && path[0] && atoll(path) > 0) {
        _flux_limit = atoll(path); 
    }
    TLOG("get cluster_updater max_per_handles=%lu", _flux_limit);

    path = ini_get_str_value("cluster_updater", "thread_num", &ctx);
    if(path && path[0]) {
        _cluster_thread_num = atol(path); 
    }
    TLOG("get cluster_updater thread_num=%d", _cluster_thread_num);

    // detail
    path = ini_get_str_value("detail", "server_port", &ctx);
    if(!path || path[0] ==0) {
        TERR("get detail server port failed");
        ini_destroy(&ctx);
        return -1;
    }
    _detail_server_port = atol(path); 

    path = ini_get_str_value("detail", "data_bak_path", &ctx);
    if(!path || path[0]==0){
        TERR("get detail data_bak_path config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    snprintf(_detail_data_path, PATH_MAX, "%s", path);
    if(make_dir(_detail_data_path, S_IRWXU | S_IRWXG | S_IRWXO)) {
        TERR("mkdir %s error, return!", _detail_data_path);
        ini_destroy(&ctx);
        return -1;
    }

    // search
    path = ini_get_str_value("search", "server_port", &ctx);
    if(!path || path[0] ==0) {
        TERR("get search server port failed");
        ini_destroy(&ctx);
        return -1;
    }
    _search_server_port = atol(path); 

    path = ini_get_str_value("search", "data_bak_path", &ctx);
    if(!path || path[0]==0){
        TERR("get search data_bak_path config error, return!");
        ini_destroy(&ctx);
        return -1;
    }
    snprintf(_search_data_path, PATH_MAX, "%s", path);
    if(make_dir(_search_data_path, S_IRWXU | S_IRWXG | S_IRWXO)) {
        TERR("mkdir %s error, return!", _search_data_path);
        ini_destroy(&ctx);
        return -1;
    }

    ini_destroy(&ctx);

    //
    FILE *fp = fopen(_cluster_conf, "rb");
    if (fp == NULL) {
        TERR("load conf fail!, conf=%s", _cluster_conf);
        return -1;
    }
    mxml_node_t* pTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
    fclose(fp);
    if (!pTree || !pTree->child) {
        TERR("mxmlLoadFile fail!, file=%s", _cluster_conf);
        return -1;
    }

    if(parseDispatcherField(pTree) < 0 || _updater_server_num <= 0) {
        TERR("get dispatcher info error, num=%d", _updater_server_num);
        return -1;
    }
    if(role != UPDATE_SERVER && parseSelfField(pTree, role) < 0) {
        TERR("get self info error");
        return -1;
    }
    mxmlDelete(pTree);
    
    if(role == DETAIL) {
        _data_path  = _detail_data_path;
    } else if(role == SEARCHER) {
        _data_path = _search_data_path;
    } else {
        _data_path = _enter_data_path;
    }

    return 0;
}

int Config::parseDispatcherField(mxml_node_t *pTree)
{
    _updater_server_num = 0;
    mxml_node_t* pNode = mxmlFindElement(pTree, pTree, "dispatcher", NULL, NULL, MXML_DESCEND);
    if(pNode == NULL) {
        TERR("cant find dispatcher table");
        return -1;
    }

    mxml_node_t *pField = mxmlFindElement(pNode, pNode, "node", NULL, NULL, MXML_DESCEND);
    while (pField != NULL) {
        if (_updater_server_num >= MAX_UPDATE_SERVER_NUM) {
            return -1;
        }
        const char *ip = mxmlElementGetAttr(pField, "ip");
        const char *port = mxmlElementGetAttr(pField, "port");
        if(NULL == ip || NULL == port) {
            TERR("dispatcher can't find ip or port");
            return -1;
        }
        _updater_server_port[_updater_server_num] = atol(port);
        snprintf(_updater_server_ip[_updater_server_num], INET_ADDRSTRLEN, "%s", ip);

        _updater_server_num++;
        pField = mxmlFindElement(pField, pNode, "node", NULL, NULL, MXML_DESCEND);
    }
    return 0;
}

int Config::parseSelfField(mxml_node_t *pNode, ROLE role)
{
    int port = 0;
    char* list_name = NULL;
    if(role == DETAIL) {
        port = _detail_server_port;
        list_name = "doc_list";
    } else if(role == SEARCHER) {
        port = _search_server_port;
        list_name = "search_list";
    } else {
        return -1;
    }

    if ((pNode = mxmlFindElement(pNode, pNode, list_name, NULL, NULL, MXML_DESCEND)) == NULL) {
        return -1;
    }

    struct ifaddrs *ifc = NULL, *ifc1 = NULL;
    if(getifaddrs(&ifc)) {
          return -1;
    }

    for(ifc1 = ifc; ifc; ifc = ifc->ifa_next) {
        if(ifc->ifa_addr == NULL || ifc->ifa_netmask == NULL)
            continue;
        inet_ntop(AF_INET, &(((struct sockaddr_in*)(ifc->ifa_addr))->sin_addr), _selfIp, INET_ADDRSTRLEN);

        if(findNode(pNode, _selfIp, port) == 0) {
            freeifaddrs(ifc1);
            return 0;
        }
    }
    freeifaddrs(ifc1);
    
    return -1;
}

int Config::findNode(mxml_node_t *pNode, const char* ip, int port)
{
    _colNum = 0;
    mxml_node_t* pField = mxmlFindElement(pNode, pNode, "col", NULL, NULL, MXML_DESCEND);
    while (pField != NULL) {
        _colNum++;
        pField = mxmlFindElement(pField, pNode, "col", NULL, NULL, MXML_DESCEND);
    }

    pField = mxmlFindElement(pNode, pNode, "col", NULL, NULL, MXML_DESCEND);
    while (pField != NULL) {
        const char *pId = mxmlElementGetAttr(pField, "id");
        _colNo = atoi(pId) - 1;
        if (_colNo >= _colNum || _colNo < 0) {
            return -1;
        }

        mxml_node_t *node = mxmlFindElement(pField, pField, "node", NULL, NULL, MXML_DESCEND);
        while (node != NULL) {
            const char *pIp = mxmlElementGetAttr(node, "ip");
            const char *pPort = mxmlElementGetAttr(node, "port");

            if(strcmp(pIp, ip) == 0 && port == atol(pPort)) {
                _selfPort = port;
                snprintf(_selfIp, INET_ADDRSTRLEN, "%s", pIp);
                return 0; 
            }

            node = mxmlFindElement(node, pField, "node", NULL, NULL, MXML_DESCEND);
        }
        
        pField = mxmlFindElement(pField, pNode, "col", NULL, NULL, MXML_DESCEND);
    }

    return -1;
}

}


