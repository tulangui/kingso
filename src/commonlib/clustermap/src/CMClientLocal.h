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
 * $Id: CMClientLocal.h 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_CLIENT_LOCAL_H_
#define _CM_CLIENT_LOCAL_H_

#include "CMTree.h"
#include "CMClientInterface.h"

namespace clustermap {

class CMClientLocal : public CMClientInterface
{
    public:
        CMClientLocal();
        virtual ~CMClientLocal();
        int32_t init(const char* local_cfg);

        int32_t Initialize(const MsgInput* in, MsgOutput* out);
        int32_t Register(const MsgInput* in, MsgOutput* out);	
        int32_t Subscriber(const MsgInput* in, MsgOutput* out);
        int32_t reportState(const MsgInput*in, MsgOutput* out);

    private:
        char m_local_cfg[PATH_MAX+1];
        CMTreeProxy* m_cpTree;
};

}
#endif
