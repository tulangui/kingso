#ifndef _MS_FLOW_CONTROL_H_
#define _MS_FLOW_CONTROL_H_

#include "merger.h"
#include "framework/Request.h"
#include "framework/Session.h"
#include "clustermap/CMClient.h"
#include "util/namespace.h"
#include "RowLocalizer.h"
#include "framework/Server.h"
#include "framework/ConnectionPool.h"
#include "MemPoolFactory.h"


//按q hash对象
namespace is_assist { class RowLocalizer; }


MERGER_BEGIN;


// 插件名获取方法
typedef const char* (merger_plugin_name_f)();
// 插件资源初始化方法
typedef void* (merger_plugin_init_f)(const char* path);
// 插件资源释放方法
typedef void (merger_plugin_dest_f)(void* handler);
// 插件数据初始化方法
typedef void* (merger_plugin_make_f)(const char* query, const int qsize, FRAMEWORK::LogInfo *pLogInfo, void* handler, MemPoolFactory* memfactory);
// 插件数据释放方法
typedef void (merger_plugin_done_f)(void* mydata);
// 插件流程控制方法
typedef int (merger_plugin_ctrl_f)(void* mydata);
// 插件请求申请方法
typedef const char* (merger_plugin_rqst_f)(void* mydata, clustermap::enodetype* nodetype);
// 结果集合并方法
typedef int (merger_plugin_cmbn_f)(void* mydata, const char* inputs[], const int inlens[], const int incnt);
// 插件结果格式化方法
typedef int (merger_plugin_frmt_f)(void* mydata, char** output, int* outlen);
// 获取插件结果格式化名称
typedef const char* (merger_plugin_ofmt_f)(void* mydata);

class FlowControl
{
    public:
        FlowControl();
        ~FlowControl();

        // 流程控制插件管理器初始化方法
        int init(const char* conf, FRAMEWORK::Server* server, MemPoolFactory* mpFactory);
        // 流程控制插件管理器释放方法
        void dest();

        // 获取插件名
        const char* name();
        // 插件数据初始化方法
        void* make(const char* query, const int qsize, FRAMEWORK::LogInfo *pLogInfo);
        // 插件数据释放方法
        void done(void* mydata);
        // 插件流程控制方法
        int ctrl(void* mydata);
        // 插件请求申请方法
        int rqst(void* mydata, FRAMEWORK::Request& req);
        // 插件结果合并方法
        int cmbn(void* mydata, const char* inputs[], const int inlens[], const int incnt);
        // 插件结果格式化方法
        int frmt(void* mydata, char* &output, int &outlen);
        // 获取插件结果格式化名称
        const char* ofmt(void* mydata);
    private:
        FRAMEWORK::Server* _server;
        // 内存池工厂类
        MemPoolFactory* _mpFactory;
        // 连接池类
        FRAMEWORK::ConnectionPool* _connPool;
        // 按行hash类
        is_assist::RowLocalizer* _row_localizer;
        // 插件句柄
        void* _plugin;
        // 插件名
        const char* _name;
        // 插件资源句柄
        void* _handler;
        // 插件资源初始化方法
        merger_plugin_init_f* _init;
        // 插件资源释放方法
        merger_plugin_dest_f* _dest;
        // 插件数据初始化方法
        merger_plugin_make_f* _make;
        // 插件数据释放方法
        merger_plugin_done_f* _done;
        // 插件流程控制方法
        merger_plugin_ctrl_f* _ctrl;
        // 插件请求申请方法
        merger_plugin_rqst_f* _rqst;
        // 插件结果合并方法
        merger_plugin_cmbn_f* _cmbn;
        // 插件结果格式化方法
        merger_plugin_frmt_f* _frmt;
        // 获取插件结果格式化名称
        merger_plugin_ofmt_f* _ofmt;
};

MERGER_END;

#endif //_MS_FLOW_CONTROL_H_
