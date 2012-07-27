/** \file
 *******************************************************************
 * $Author: santai.ww $
 *
 * $LastChangedBy: santai.ww $
 *
 * $Revision: 516 $
 *
 * $LastChangedDate: 2011-04-08 23:43:26 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: CMClientLocal.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "CMTree.h"
#include "CMClientLocal.h"

namespace clustermap {

CMClientLocal::CMClientLocal()
{
    //printf("CMClientLocal new\n");
    m_cpTree = NULL;
}

CMClientLocal::~CMClientLocal()
{
    //printf("CMClientLocal del\n");
    if(m_cpTree)
        delete m_cpTree;
    m_cpTree = NULL;
}

int32_t CMClientLocal::init(const char* local_cfg)
{
    if(local_cfg == NULL)
        return -1;

    strcpy(m_local_cfg, local_cfg);

    return 0;
}

int32_t CMClientLocal::Initialize(const MsgInput* in, MsgOutput* out)
{
    if(m_cpTree) delete m_cpTree;
    m_cpTree = new CMTreeProxy();
    if(m_cpTree == NULL)
        return -1;
    if(m_cpTree->init(m_local_cfg, 0))
        return -1;

    InitInput* input = (InitInput*)in;
    InitOutput* output = (InitOutput*)out;

    if(m_cpTree->process(input, output) != 0)
        return -1;
    return 0;
}

int32_t CMClientLocal::Register(const MsgInput* in, MsgOutput* out)
{
    if(m_cpTree == NULL)
        return -1;
    if(m_cpTree->process(in, out) != 0)
        return -1;
    return 0;
}

int32_t CMClientLocal::Subscriber(const MsgInput* in, MsgOutput* out)
{
    if(m_cpTree == NULL)
        return -1;
    if(m_cpTree->process(in, out) != 0)
        return -1;
    return 0;
}

int32_t CMClientLocal::reportState(const MsgInput*in, MsgOutput* out)
{
    return 0;
}

}
