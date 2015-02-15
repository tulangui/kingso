/** \file
 *******************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 11552 $
 *
 * $LastChangedDate: 2012-04-25 11:53:21 +0800 (Wed, 25 Apr 2012) $
 *
 * $Id: CMClientInterface.cpp 11552 2012-04-25 03:53:21Z pujian $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "CMClientInterface.h"

namespace clustermap {

ClientNodeInfo::ClientNodeInfo(CMClientInterface* head)
{
    //printf("CMClientNodeInfo new\n");
    type_para_table[blender] = 0;
    type_para_table[merger] = 0;
    type_para_table[search] = 0;
    type_para_table[search_mix] = 0;
    type_para_table[doc_mix] = 1;
    type_para_table[search_doc] = 1;

    cpMISNode = NULL;
    cpCluster = NULL;
    cppNodeCol = NULL;
    max_row_num = NULL;
    cpRelation = NULL;

    next = NULL;
    _head = head;

    max_node_num = 0;
    max_col_num = 0;
    _max_row_num = 0;
    max_relation_num = 0;
    _isSubscribered = false;
    pthread_mutex_init(&lock, NULL);
    pthread_rwlock_init(&_lock_EnvVar, NULL);
}

ClientNodeInfo::~ClientNodeInfo()
{
    //printf("CMClientNodeInfo del\n");
    max_node_num = 0;
    if(cpMISNode)
        delete[] cpMISNode;
    cpMISNode = NULL;

    max_col_num = 0;
    if(cpCluster)
        delete[] cpCluster;
    cpCluster = NULL;

    if(cppNodeCol)
        delete[] cppNodeCol;
    cppNodeCol = NULL;

    if(max_row_num)
        delete[] max_row_num;
    max_row_num = NULL;

    max_relation_num = 0;
    if(cpRelation)
        delete[] cpRelation;
    cpRelation = NULL;

    clusterNameTable.clear();

    pthread_mutex_destroy(&lock);
    pthread_rwlock_destroy(&_lock_EnvVar);
}

int32_t ClientNodeInfo::process(const MsgInput* in, MsgOutput* out)
{
    switch (in->type) {
        case msg_init:
            return _init(in, out);
        case msg_register:
            return _register(in, out);
        case msg_sub_child:
        case msg_sub_all:
        case msg_sub_selfcluster:
            return _subscriber(in, out);
        case cmd_alloc_sub:
            return allocSubRow(in, out);
        case cmd_alloc_selfcluster:
            return allocSelfCluster(in, out);
        case cmd_alloc_subcluster:
            return allocSubCluster(in, out);
        case cmd_get_info:
            return getNodeInfo(in, out);
        case cmd_alloc_node_by_node:
        case cmd_alloc_node_by_name:
            return allocNode(in, out);
        case cmd_set_state:
            return setSubClusterStation(in, out);
        case cmd_set_state_all_normal:
            return setAllStateNormal(in, out);
        case cmd_display_all:
            return display(in, out);
        case cmd_get_tree:
            return getTree(in, out);
        case cmd_get_ClusterCount_ByStatus:
            return getClusterCountByStatus(in, out);
        case cmd_get_GlobalEnvVar:
            return getGlobalEnvVar(in, out);
        case cmd_get_clustercount:
            return getSubClusterCount(in, out);
        case cmd_get_allnodes_col:
            return getAllNodesInOneSubCluster(in, out);
        case cmd_get_normalnodes_col:
            return getNormalNodesInOneSubCluster(in, out);
        default:
            return -1;
    };
    return 0;
}

int32_t ClientNodeInfo::_init(const MsgInput* in, MsgOutput* out)
{
    InitInput* input = (InitInput*)in;
    InitOutput* output = (InitOutput*)out;

    CMClientInterface* head = _head;

    int32_t ret = -1;
    while(head) {
        ret = head->Initialize(input, output);
        if(ret == 0)
            break;
        head = head->next;
    }
    if(ret == -1)
        return -1;

    char* p = out->outbuf+1;
    p += self_cluster.bufToNode(p);
    p += self_node.bufToNode(p);

    self_node.m_clustername = self_cluster.m_name;

    return 0;
}

bool ClientNodeInfo::isMatchNodeType(enodetype ExpectedType, enodetype CurrentType)
{
    if (ExpectedType == CurrentType) {
        return true;
    } else if (ExpectedType == search_mix && CurrentType == search) {
        return true;
    } else if (ExpectedType == search_doc && CurrentType == search_mix) {
        return true;
    } else if (ExpectedType == search && CurrentType == search_mix) {
        return true;
    } else if (ExpectedType == doc_mix && (CurrentType == search_doc || CurrentType == search_mix)) {
        return true;
    }
    return false;
}

int32_t ClientNodeInfo::allocSubRow(const MsgInput* in, MsgOutput* out)
{
    if (!_isSubscribered) { // Subscriber failed
        return -1;
    }
    allocSubInput* input = (allocSubInput*)in;
    allocSubOutput* output = (allocSubOutput*)out;

    enodetype nodetype = input->node_type;
    int32_t *&ids = output->ids;
    int32_t &size = output->size;

    if (input->logic_op == UNKNOWN 
        || input->logic_op == LE && input->level < _min_level 
        || input->logic_op == GE && input->level > _max_level
        || input->logic_op == EQ && input->level > _max_level 
        || input->level <= 0) {
        return -1;
    }
    ids = (int32_t*)malloc(sizeof(int32_t) * max_col_num);
    if(ids == NULL)
        return -1;

    int32_t* p = ids, max_num, *use;
    int32_t* matchfields = NULL;
    if(input->locating_signal > 0) {
        matchfields = new int32_t[_max_row_num];
        if(matchfields == NULL)
            return -1;
    }
    for(int i = 0; i < max_col_num; i++)
    {
        if (input->logic_op == EQ) {
            if(cpCluster[i].level != input->level)
                continue;
        } else if (input->logic_op == GE) {
            if(cpCluster[i].level < input->level)
                continue;
        } else if (input->logic_op == LE) {
            if(cpCluster[i].level > input->level)
                continue;
        } else {
            continue;
        }
        max_num = max_row_num[i];
        use = &cur_row_use[0][i];

        *p = -1;
        pthread_mutex_lock(&lock);
        if(input->locating_signal > 0) {
            int matchnum = 0;
            for(int j = 0; j < max_num; j++) {
                if (!isMatchNodeType(nodetype, cppNodeCol[i][j].m_type))
                    continue;
                matchfields[matchnum++] = j;
            }
            if(matchnum > 0) {
                *p = matchfields[input->locating_signal%matchnum];
                if(cppNodeCol[i][*p].m_state.state) *p = -1;
            }
        }
        if (cpCluster[i].m_state_cluster == GREEN) {
            if(*p != -1) {
                *p = cppNodeCol[i][*p].m_localid;
                p++;
            } else {
                *p = cppNodeCol[i][0].m_localid & 0xffff0000 | 0x0000ffff;  // invalid localid as default
                for(int j = 0; j < max_num; j++, (*use)++)
                {
                    if(*use >= max_num) *use = 0;
                    if(cppNodeCol[i][*use].m_state.state)
                        continue;
                    if (!isMatchNodeType(nodetype, cppNodeCol[i][*use].m_type))
                        continue;
                    *p = cppNodeCol[i][*use].m_localid;
                    (*use)++;
                    break;
                }
                p++;
            }
        } else if (cpCluster[i].m_state_cluster == YELLOW) {
            if(*p != -1) {
                *p = cppNodeCol[i][*p].m_localid;
                p++;
            } else {
                *p = cppNodeCol[i][0].m_localid & 0xffff0000 | 0x0000ffff;  // invalid localid as default
                if(input->locating_signal <= 0) {
                    for(int j = 0; j < max_num; j++, (*use)++)
                    {
                        if(*use >= max_num) *use = 0;
                        if (!isMatchNodeType(nodetype, cppNodeCol[i][*use].m_type))
                            continue;
                        if (cppNodeCol[i][*use].m_state.state == normal)
                            *p = cppNodeCol[i][*use].m_localid;
                        (*use)++;
                        break;
                    }
                }
                p++;
            }
        } else if (cpCluster[i].m_state_cluster == RED) {
            *p = cppNodeCol[i][0].m_localid & 0xffff0000 | 0x0000ffff;  // invalid localid as default
            p++;
        } else {
            *p = cppNodeCol[i][0].m_localid & 0xffff0000 | 0x0000ffff;  // invalid localid as default
            p++;
            TERR("in ClientNodeInfo::allocSubRow() cpCluster[i].m_state_cluster ERROR");
        }

        pthread_mutex_unlock(&lock);	
    }
    size = p - ids;
    if(matchfields) delete[] matchfields;

    return 0;
}

int32_t ClientNodeInfo::allocSelfCluster(const MsgInput* in, MsgOutput* out)
{
    allocSubInput* input = (allocSubInput*)in;
    allocSubOutput* output = (allocSubOutput*)out;

    enodetype nodetype = input->node_type;
    int32_t *&ids = output->ids;
    int32_t &size = output->size;

    if (input->logic_op == UNKNOWN 
        || input->logic_op == LE && input->level < _min_level 
        || input->logic_op == GE && input->level > _max_level
        || input->logic_op == EQ && input->level > _max_level 
        || input->level <= 0) {
        return -1;
    }
    int32_t *p, max_num;
    for(int i = 0; i < max_col_num; i++)
    {
        if (input->logic_op == EQ) {
            if(cpCluster[i].level != input->level)
                continue;
        } else if (input->logic_op == GE) {
            if(cpCluster[i].level < input->level)
                continue;
        } else if (input->logic_op == LE) {
            if(cpCluster[i].level > input->level)
                continue;
        } else {
            continue;
        }
        max_num = max_row_num[i];
        ids = (int32_t*)malloc(sizeof(int32_t) * max_num);
        if (ids == NULL)
            return -1;
        p = ids;
        pthread_mutex_lock(&lock);
        for (int j = 0; j < max_num; j++)
        {
            if (isMatchNodeType(nodetype, cppNodeCol[i][j].m_type)) {
                *p++ = cppNodeCol[i][j].m_localid;
                //printf("SelfCluster Node = %u\n", cppNodeCol[i][j].m_port);
            }
        }
        pthread_mutex_unlock(&lock);	
    }
    size = p - ids;

    return 0;
}

int32_t ClientNodeInfo::allocSubCluster(const MsgInput* in, MsgOutput* out)
{
    allocSubInput* input = (allocSubInput*)in;
    allocSubOutput* output = (allocSubOutput*)out;

    enodetype nodetype = input->node_type;
    int32_t *&ids = output->ids;
    int32_t &size = output->size;

    ids = (int32_t*)malloc(sizeof(int32_t) * max_col_num);
    if(ids == NULL)
        return -1;

    int32_t* p = ids, max_num, *use;
    for(int i = 0; i < max_col_num; i++)
    {
        max_num = max_row_num[i];
        use = &cur_row_use[0][i];
        pthread_mutex_lock(&lock);
        *p = cppNodeCol[i][0].m_localid & 0xffff0000 | 0x0000ffff;  // invalid localid as default
        for(int j = 0; j < max_num; j++, (*use)++)
        {
            if(*use >= max_num) *use = 0;
            if (!isMatchNodeType(nodetype, cppNodeCol[i][*use].m_type))
                continue;
            *p = cppNodeCol[i][*use].m_localid;
            (*use)++;
            break;
        }
        p++;
        pthread_mutex_unlock(&lock);	
    }
    size = p - ids;

    return 0;
}

int32_t ClientNodeInfo::getNodeInfo(const MsgInput* in, MsgOutput* out)
{
    getNodeInput* input = (getNodeInput*)in;
    getNodeOutput* output = (getNodeOutput*)out;
    int32_t nodeid = input->local_id;

    if(nodeid < 0) {
        output->cpNode = &self_node;
    } else {
        int32_t col = nodeid>>16;
        int32_t row = nodeid&0xff;
        if(col < 0 || col >= max_col_num || row < 0 || row >= max_row_num[col])
            output->cpNode = NULL;
        else
            output->cpNode = &cppNodeCol[col][row];
    }
    return 0;
}

int32_t ClientNodeInfo::allocNode(const MsgInput* in, MsgOutput* out)
{
    allocNodeInput* input = (allocNodeInput*)in;
    allocNodeOutput* output = (allocNodeOutput*)out;

    CMISNode* node = input->cpNode;
    enodetype nodetype = input->node_type;
    bool bSubClusters = false;

    if(input->type == cmd_alloc_node_by_name) {
        if(input->name == NULL)
            return -1;
        std::map<std::string, CMISNode*>::iterator iter = clusterNameTable.find(input->name);
        if(iter == clusterNameTable.end())
            return -1;
        node = iter->second;
        if (strstr(input->name, "tcp:") == NULL && strstr(input->name, "udp:") == NULL)
            bSubClusters = true;
    }

    int32_t col = node->m_localid>>16;
    int32_t row = node->m_localid&0xff;
    if(col < 0 || col >= max_col_num || row < 0 || row >= max_row_num[col])
        return -1;
    if(!bSubClusters && (cpCluster[col].docsep == false || nodetype == self))  {
        output->cpNode = &cppNodeCol[col][row];
        return 0;
    }

    int32_t max_num = max_row_num[col];
    int32_t* use = &cur_row_use[1][col];

    pthread_mutex_lock(&lock);
    for(int i = 0; i < max_num; i++, (*use)++){
        if(*use >= max_num) *use = 0;
        node = &cppNodeCol[col][*use];
        if(node->m_state.state)
            continue;
        if (!isMatchNodeType(nodetype, node->m_type))
            continue;
        (*use)++;
        pthread_mutex_unlock(&lock);
        output->cpNode = node;
        return 0;
    }
    pthread_mutex_unlock(&lock);

    return -1;
}

int32_t ClientNodeInfo::setSubClusterStation(const MsgInput* in, MsgOutput* out)
{
    setStatInput* input = (setStatInput*)in;

    char* stateBitmap = input->stateBitmap;
    int32_t node_count = input->node_num;
    if(node_count < max_node_num + max_col_num)
        return -1;

    //printf("\n%u sub:", self_node.m_port);
    CMISNode* cpNode = cpMISNode;
    for(int32_t i = 0; i < max_node_num; i++, cpNode++) {
        if(cpNode->m_startTime <= 0) {
            if(stateBitmap[i] == 0)
                cpNode->m_startTime = time(NULL);
        } 
        if(cpNode->m_startTime <= 0)
            continue;
        if(cpNode->m_state.laststate == uninit) {
            if(stateBitmap[i]) {
                cpNode->m_dead_begin = time(NULL);
                cpNode->m_dead_end = 0;
            }
        } else if(cpNode->m_state.laststate != stateBitmap[i]) {
            if(stateBitmap[i] == 0) {
                cpNode->m_dead_end = time(NULL);
                cpNode->m_dead_time += cpNode->m_dead_end - cpNode->m_dead_begin;
            } else {
                cpNode->m_dead_begin = time(NULL);
                cpNode->m_dead_end = 0;
            }
        }																		 
        cpNode->m_state.state = stateBitmap[i];
        cpNode->m_state.laststate = stateBitmap[i];
        //printf(" %ustate=%d", cpNode->m_port, stateBitmap[i]);
    }
    //printf("\n");
    for (int32_t i = 0; i < max_col_num; i++) {
        cpCluster[i].m_state_cluster = (estate_cluster)stateBitmap[max_node_num + i];
    }
    pthread_rwlock_wrlock(&_lock_EnvVar);
    m_EnvVarMap.clear();
    char *pchEnvVar_1 = NULL;
    char *pchEnvVar_2 = NULL;
    char *pchEnvVar_3 = NULL;
    int32_t count_EnvVar = max_node_num + max_col_num;
    // CMTree::getStateUpdate()     "^%s=%s$", iter->first.c_str(), iter->second.c_str()
    while ((pchEnvVar_1 = strchr(stateBitmap + count_EnvVar, '^'))) {
        if ((pchEnvVar_2 = strchr(stateBitmap + count_EnvVar, '=')) == NULL) {
            count_EnvVar++;
            continue;
        }        
        if ((pchEnvVar_3 = strchr(stateBitmap + count_EnvVar, '$')) == NULL) {
            count_EnvVar++;
            continue;
        }
        *pchEnvVar_2 = 0;
        *pchEnvVar_3 = 0;
        m_EnvVarMap[std::string(pchEnvVar_1 + 1)] = std::string(pchEnvVar_2 + 1);
        count_EnvVar += pchEnvVar_3 - pchEnvVar_1 + 1; 
    }
    /*
       TLOG("m_EnvVarMap begin");
       for (map<string, string>::iterator iter = m_EnvVarMap.begin(); iter != m_EnvVarMap.end(); iter++) {
       TLOG("%s=%s", iter->first.c_str(), iter->second.c_str());
       }
       TLOG("m_EnvVarMap end");
     */
    pthread_rwlock_unlock(&_lock_EnvVar);
    return 0;
}

int32_t ClientNodeInfo::setAllStateNormal(const MsgInput* in, MsgOutput* out)
{
    CMISNode* cpNode = cpMISNode;
    for(int32_t i = 0; i < max_node_num; i++, cpNode++)
        cpNode->m_state.state = 0;
    for (int32_t i = 0; i < max_col_num; i++) {
        cpCluster[i].m_state_cluster = GREEN;
    }   
    return 0;
}

int32_t ClientNodeInfo::_register(const MsgInput* in, MsgOutput* out)
{
    int32_t ret = -1;
    CMClientInterface* cpClient = _head;
    while(cpClient) {
        if(cpClient->Register(in, out) == 0)
            ret = 0;
        cpClient = cpClient->next;
    }
    return ret;

}
int32_t ClientNodeInfo::_subscriber(const MsgInput* in, MsgOutput* out)
{
    int32_t ret_value = -1;
    CMClientInterface* cpClient = _head;
    while(cpClient) {
        if(cpClient->Subscriber(in, out) == 0) {
            ret_value = 0;
            break;
        }
        cpClient = cpClient->next;
    }
    if(ret_value != 0)
        return -1;

    SubOutput* output = (SubOutput*)out;
    char* p = output->outbuf+1;
    max_col_num = ntohl(*(int32_t*)p);
    p += 4;
    max_node_num = ntohl(*(int32_t*)p);
    p += 4;
    max_relation_num = ntohl(*(int32_t*)p);
    p += 4;
    output->tree_time = ntohl(*(int32_t*)p);
    p += 4;
    output->tree_no = ntohl(*(int32_t*)p);
    p += 4;
    if(max_col_num <= 0 || max_node_num <= 0)
        return -1;

    cpMISNode = new CMISNode[max_node_num];
    cpCluster = new CMCluster[max_col_num];
    cppNodeCol = new CMISNode*[max_col_num];
    max_row_num = new int32_t[max_col_num*3];
    if(!cpMISNode || !cpCluster || !cppNodeCol || !max_row_num)
        return -1;
    memset(max_row_num, 0, max_col_num*3*4);
    cur_row_use[0] = max_row_num + max_col_num;
    cur_row_use[1] = cur_row_use[0] + max_col_num;
    _min_level = 0xfffff;
    _max_level = -1;
    char name[128];
    CProtocol cProtocol;
    CMISNode* pNode = cpMISNode;
    std::map<std::string, CMCluster*> clustermap;
    for(int32_t i = 0; i < max_col_num; i++) {
        p += cpCluster[i].bufToNode(p);
        cpCluster[i].m_cmisnode = pNode;
        if(_min_level > cpCluster[i].level)
            _min_level = cpCluster[i].level;
        if(_max_level < cpCluster[i].level)
            _max_level = cpCluster[i].level;
        cppNodeCol[i] = pNode;
        max_row_num[i] = cpCluster[i].node_num;

        if(max_row_num[i] > _max_row_num)
            _max_row_num = max_row_num[i];

        clusterNameTable[cpCluster[i].m_name] = pNode;
        clustermap[cpCluster[i].m_name] = cpCluster + i;
        //FILE* fp = fopen("subNode.txt", "a");
        for(int32_t j = 0; j < max_row_num[i]; j++, pNode++) {
            p += pNode->bufToNode(p);
            //fprintf(fp, "port=%u,state=%d,laststate=%d,deadbegin=%u,dead_end=%u\n", pNode->m_port, pNode->m_state.state, pNode->m_state.laststate,pNode->m_dead_begin, pNode->m_dead_end); 
            pNode->m_localid = (i<<16) | j;
            pNode->m_clustername = cpCluster[i].m_name;
            sprintf(name, "%s:%s:%u", cProtocol.protocolToStr(pNode->m_protocol), pNode->m_ip, pNode->m_port);
            clusterNameTable[name] = pNode;
            //printf("%u subsciber %d %d = %s type %d\n", self_node.m_port, i, j, name, pNode->m_type);
        }
        //fclose(fp);
    }

    if(max_relation_num > 0) {
        cpRelation = new CMRelation[max_relation_num];
        if(cpRelation == NULL)
            return -1;
        CMRelation* p1 = cpRelation;
        std::map<std::string, CMRelation*> relationmap;
        char* pbak = p;
        for(int32_t i = 0; i < max_relation_num; i++, p1++, p += 64*4) {
            if(p[0]) p1->cpCluster = clustermap[p];
            relationmap[p] = p1;
        }
        p = pbak;
        p1 = cpRelation;
        for(int32_t i = 0; i < max_relation_num; i++, p1++) {
            p += 64;
            if(p[0]) p1->parent = relationmap[p];
            p += 64;
            if(p[0]) p1->child = relationmap[p];
            p += 64;
            if(p[0]) p1->sibling = relationmap[p];
            p += 64;
        }
    }
    _isSubscribered = true;
    return 0;
}
int32_t ClientNodeInfo::display(const MsgInput* in, MsgOutput* out)
{
    displayOutput* output = (displayOutput*)out;

    output->node_num = max_node_num;
    output->cpNode = new CMISNode[max_node_num];
    if(output->cpNode == NULL)
        return -1;
    for(int32_t i = 0; i < max_node_num; i++) {
        output->cpNode[i] = cpMISNode[i];
        if(cpMISNode[i].sub_child) {
            output->cpNode[i].sub_child = new CSubInfo;
            output->cpNode[i].sub_child = cpMISNode[i].sub_child;
        }
    }
    return 0;
}
int32_t ClientNodeInfo::getTree(const MsgInput* input, MsgOutput* output)
{
    getTreeInput* in = (getTreeInput*)input;
    getTreeOutput* out = (getTreeOutput*)output;

    if(in->tree == NULL) {
        in->tree = new CMTree();
        if(in->tree == NULL)
            return -1;
    }
    if(in->tree->init(this, in->tree_time, in->tree_no) != 0)
        return -1;
    out->tree = in->tree;

    return 0;
}

int32_t ClientNodeInfo::getClusterCountByStatus(const MsgInput* input, MsgOutput* output)
{
    getClusterCountByStatusInput* in = (getClusterCountByStatusInput*)input;
    getClusterCountByStatusOutput* out = (getClusterCountByStatusOutput*)output;

    uint32_t count = 0;
    for (int i = 0; i < max_col_num; i++) {
        if (cpCluster[i].m_state_cluster == in->state_cluster)
            ++count;
    }
    out->ClusterCount = count;

    return 0;
}

int32_t ClientNodeInfo::getGlobalEnvVar(const MsgInput* input, MsgOutput* output)
{
    getGlobalEnvVarInput* in = (getGlobalEnvVarInput*)input;
    getGlobalEnvVarOutput* out = (getGlobalEnvVarOutput*)output;

    pthread_rwlock_rdlock(&_lock_EnvVar);
    std::map<std::string, std::string>::iterator iter = m_EnvVarMap.find(std::string(in->EnvVar_Key));
    if (iter == m_EnvVarMap.end())
        out->EnvVar_Value = "nil";
    else
        out->EnvVar_Value = iter->second;
    pthread_rwlock_unlock(&_lock_EnvVar);
    return 0;
}

int32_t ClientNodeInfo::getSubClusterCount(const MsgInput* in, MsgOutput* out)
{
    if (!_isSubscribered) { // Subscriber failed
        return -1;
    }
    //getNodesInput* input = (getNodesInput*)in;
    getNodesOutput* output = (getNodesOutput*)out;
//    enodetype nodetype = input->node_type;
    output->size = max_col_num;
    return 0;
}

int32_t ClientNodeInfo::getAllNodesInOneSubCluster(const MsgInput* in, MsgOutput* out)
{
    if (!_isSubscribered) { // Subscriber failed
        return -1;
    }
    getNodesInput* input = (getNodesInput*)in;
    getNodesOutput* output = (getNodesOutput*)out;
    enodetype nodetype = input->node_type;
    int32_t subcluster_num = input->subcluster_num;
    if (subcluster_num < 0 || subcluster_num >= max_col_num) {
        return -1;
    }
    int32_t row_count = max_row_num[subcluster_num];
    int32_t *&ids = output->ids;
    int32_t &size = output->size;
    ids = (int32_t*)malloc(sizeof(int32_t) * row_count);
    if (ids == NULL) {
        return -1;
    }
    int32_t *p = ids;
    pthread_mutex_lock(&lock);
    *p = cppNodeCol[subcluster_num][0].m_localid & 0xffff0000 | 0x0000ffff;  // invalid localid as default
    for (int j = 0; j < row_count; j++) {
        if (!isMatchNodeType(nodetype, cppNodeCol[subcluster_num][j].m_type)) {
            continue;
        }
        *p++ = cppNodeCol[subcluster_num][j].m_localid;
    }
    pthread_mutex_unlock(&lock);	
    size = p - ids;
    return 0;
}

int32_t ClientNodeInfo::getNormalNodesInOneSubCluster(const MsgInput* in, MsgOutput* out)
{
    if (!_isSubscribered) { // Subscriber failed
        return -1;
    }
    getNodesInput* input = (getNodesInput*)in;
    getNodesOutput* output = (getNodesOutput*)out;
    enodetype nodetype = input->node_type;
    int32_t subcluster_num = input->subcluster_num;
    if (subcluster_num < 0 || subcluster_num >= max_col_num) {
        return -1;
    }
    int32_t row_count = max_row_num[subcluster_num];
    int32_t *&ids = output->ids;
    int32_t &size = output->size;
    ids = (int32_t*)malloc(sizeof(int32_t) * row_count);
    if (ids == NULL) {
        return -1;
    }
    int32_t *p = ids;
    pthread_mutex_lock(&lock);
    *p = cppNodeCol[subcluster_num][0].m_localid & 0xffff0000 | 0x0000ffff;  // invalid localid as default
    for (int j = 0; j < row_count; j++) {
        if (!isMatchNodeType(nodetype, cppNodeCol[subcluster_num][j].m_type)) {
            continue;
        }
        if (cppNodeCol[subcluster_num][j].m_state.state) {
            continue;
        }
        *p++ = cppNodeCol[subcluster_num][j].m_localid;
    }
    pthread_mutex_unlock(&lock);	
    size = p - ids;
    return 0;
}

CMClientProxy::CMClientProxy()
{
    //printf("CMClientProxy new\n");
    _res = NULL;
}
CMClientProxy::~CMClientProxy()
{
    //printf("CMClientProxy del\n");
    ClientNodeInfo* next = _res;
    while(next) {
        next = _res->next;
        delete _res;
        _res = next;
    }
}

int32_t CMClientProxy::Initialize(const CMClientInterface* cpHead, const MsgInput* in)
{
    _head = (CMClientInterface*)cpHead;
    ClientNodeInfo* res = new (std::nothrow) ClientNodeInfo(_head);
    if(res == NULL)
        return -1;
    res->next = _res;
    _res = res;

    _init_in = *(InitInput*)in;
    WR_LOCK(_lock);
    if(!_res) return -1;

    if(_init_in.ip[0]) {
        return _res->process(&_init_in, &_init_out);
    } else {
        struct ifaddrs *ifc = NULL, *ifc1 = NULL;
        if(getifaddrs(&ifc))
            return -1;
        for(ifc1 = ifc; ifc; ifc = ifc->ifa_next) {
            if(ifc->ifa_addr == NULL || ifc->ifa_netmask == NULL)
                continue;
            inet_ntop(AF_INET, &(((struct sockaddr_in*)(ifc->ifa_addr))->sin_addr), _init_in.ip, 24);
            if(_res->process(&_init_in, &_init_out) == 0) {
                strcpy(((InitInput*)in)->ip, _init_in.ip);	
                break;
            }
        }
        freeifaddrs(ifc1);
        if(ifc == NULL) return -1;
    }
    WR_UNLOCK(_lock);

    return 0;
}

int32_t CMClientProxy::Register(const MsgInput* in)
{
    _reg_in = *(RegInput*)in;

    RD_LOCK(_lock);
    if(!_res) return -1;
    return _res->process(&_reg_in, &_reg_out);
    RD_UNLOCK(_lock);

    return -1;
}

int32_t CMClientProxy::Subscriber(const MsgInput* in)
{
    _sub_in = *(SubInput*)in;
    _sub_in.op_time = time(NULL);

    RD_LOCK(_lock);
    if(!_res) return -1;
    return _res->process(&_sub_in, &_sub_out);
    RD_UNLOCK(_lock);

    return -1;
}

int32_t CMClientProxy::process(const MsgInput* in, MsgOutput* out)
{
    RD_LOCK(_lock);
    if(!_res) return -1;
    return _res->process(in, out);
    RD_UNLOCK(_lock);

    return -1;
}
bool CMClientProxy::reload()
{
    ClientNodeInfo* res = new (std::nothrow) ClientNodeInfo(_head);
    if(!res) {
        TLOG("reload -- alloc New ClientNodeInfo error");
        return false;
    }
    if(_init_in.type != msg_unvalid) {
        if(res->process(&_init_in, &_init_out) != 0 && _sub_in.type != msg_sub_all) {
            TLOG("reload -- init error");
            delete res;
            return false;
        }
    }
    if(_reg_in.type != msg_unvalid) {
        if(res->process(&_reg_in, &_reg_out) != 0) {
            TLOG("reload -- reg error");
            delete res;
            return false;
        }
    }
    if(_sub_in.type != msg_unvalid) {
        _sub_in.op_time = time(NULL);
        if(res->process(&_sub_in, &_sub_out) != 0) {
            TLOG("reload -- sub error");
            delete res;
            return false;
        }
    }

    WR_LOCK(_lock);
    res->next = _res;
    _res = res;
    WR_UNLOCK(_lock);
    TLOG("reload success --- %d", getpid());

    return true;
}

CMClientInterface::CMClientInterface()
{
    //printf("CMClientInterface new\n");
    next = NULL;
}
CMClientInterface::~CMClientInterface()
{
    //printf("CMClientInterface del\n");
}

}
