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
 * $Id: Command.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: command which control server status (start stop restart)$
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_COMMAND_H_
#define _FRAMEWORK_COMMAND_H_
#include "util/common.h"
#include "framework/namespace.h"
#include <signal.h>

FRAMEWORK_BEGIN;

class Server;
class Args;

class CommandLine {
    public:
        CommandLine(const char *name);
        virtual ~CommandLine();
    public:
        /**
         *@brief 运行命令
         *@return 0,成功 非0,失败
         */
        int run(int argc, char *argv[]);
        /**
         *@brief 结束命令
         */
        void terminate();
    public:
        /**
         *@brief 注册信号
         */
        static void registerSig();
        /**
         *@brief 等待信号触发
         */
        void waitSig();
    private:
        void help(int type = 0);
        void showShortVersion();
        void showLongVersion();
    protected:
        virtual Server *makeServer() = 0;
    private:
	//运行程序名称
        char *_name;
	//服务句柄
        Server *_server;
	//信号掩码
        sigset_t _mask;
    public:
        CommandLine *_next;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_COMMAND_H_

