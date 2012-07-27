/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 167 $
 *
 * $LastChangedDate: 2011-03-25 11:13:57 +0800 (Fri, 25 Mar 2011) $
 *
 * $Id: WebProbe.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: the probe of service $
 *
 *******************************************************************
 */

#ifndef _WEB_PROBE_H_
#define _WEB_PROBE_H_

#include <stdio.h>
#include <string>
#include <mxml.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "framework/TaskQueue.h"
#include "framework/Session.h"

FRAMEWORK_BEGIN;

class WebProbe
{
    public:
        WebProbe();
        virtual ~WebProbe();
        /**
         *@brief 初始化WebProbe
         *@param httpRoot 文件访问根目录
         *@return 0,成功 非0,失败
         */
        int32_t initialize(const char *httpRoot);
        //thread hook
        /**
         *@brief 线程句柄
         *@param args 输入参数
         */
        static void *threadHook(void *args);
        //Deal with the probe request
        /**
         *@brief Deal with the probe request
         */
        virtual void *run();
        //是否是探测报文
        /**
         *@brief 是否是探测的请求
         *@return true,是 false,否
         */
        bool isProbeRequest(const char *uri);
        //把报文加入队列中
        /**
         *@brief 把报文加入队列中
         *@param session 会话对象
         *@return 0,成功 非0,失败
         */
        int32_t enQueue(Session *session) 
        { 
            return _probeQueue->enqueue(session); 
        }
        //从队列中取出报文
        /**
         *@brief 从队列中取出报文
         *@param session 会话对象
         *@return 0,成功 非0,失败
         */
        int32_t deQueue(Session **session) 
        { 
            return _probeQueue->dequeue(session); 
        } 
        //停止服务
        /**
         *@brief 停止服务
         */
        void terminate();

    private:
        //prob任务队列
        TaskQueue *_probeQueue;
        std::string _httpRoot;
        pthread_t _pid;
        bool _terminated;
};

FRAMEWORK_END;

#endif

