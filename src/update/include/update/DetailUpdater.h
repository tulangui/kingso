#ifndef _DETAILUPDATER_H_
#define _DETAILUPDATER_H_

#include "update/Updater.h"
#include "update/update_def.h"
#include "commdef/DocMessage.pb.h"

UPDATE_BEGIN

class DetailUpdater : public Updater 
{ 
public:
    DetailUpdater(); 
    ~DetailUpdater(); 

    /*
     * 初始化函数，按照框架的要求，完成自身初始化，增量数据恢复
     * @param pConf 配置文件路径
     * @return 操作是否成功，0为成功，<0失败
     */
    int init(const char* conf_path);

private:
    /*
     * 分配、释放更新资源 
     */
    UpdateRes* allocRes();
    void freeRes(UpdateRes* res);
    
    /*
     * 检查更新消息完整性,并解析
     * @param res 更新消息
     * success: 0, failed: other
     */
    int check(UpdateRes* res);

    /*
     * 实现底层更新接口调用
     * @param res 更新消息
     * success: 0, failed: other
     */
    int doupdate(UpdateRes* res);
    
    /*
     * 获取当前资源的信息
     */
    int64_t nid(UpdateRes* res);
    UpdateAction action(UpdateRes* res);
};

UPDATE_END

#endif 
