#ifndef _UPDATER_H_
#define _UPDATER_H_

#include "util/ThreadLock.h"
#include "update/update_def.h"

UPDATE_BEGIN 

struct UpdateRes;
class UpdateProcessor;

enum ROLE {
    DETAIL = 0,         // detail 
    SEARCHER,           // searcher
    UPDATE_SERVER,      // updater
    ROLE_LAST,          // 最后一个角色
};

class Updater 
{ 
public:
    Updater();
    virtual ~Updater();

    /*
     * 初始化函数，按照框架的要求，完成自身初始化，增量数据恢复
     * @param pConf 配置文件路径
     * @return 操作是否成功，0为成功，<0失败
     */
    virtual int init(const char* conf_path);

    /*
     * 启动服务
     * @param arg: Updater对象实例
     * @return 操作是否成功, NULL 失败
     */ 
    static void* start(void* arg);

    /*
     * 停止服务
     */
    void stop();

public: 
    /*
     * 分配、释放更新资源 
     */
    virtual UpdateRes* allocRes() = 0;
    virtual void freeRes(UpdateRes* res) = 0;

    /*
     * 检查更新消息完整性
     */
    virtual int check(UpdateRes* res) = 0;

    /*
     * 底层调用
     */
    virtual int doupdate(UpdateRes* res) = 0;

    /*
     * 获取当前资源的信息
     */
    virtual int64_t nid(UpdateRes* res) = 0;
    virtual UpdateAction action(UpdateRes* res) = 0;

protected:
    ROLE _rol;                  // 自身角色
    util::Mutex _lock;      
    uint64_t _incMsgCount;      // 更新消息数量
    UpdateProcessor* _processor;// 更新执行体
};

UPDATE_END


#endif 
