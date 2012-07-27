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
 * $Id: CMClientNodeInfoMain.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

namespace clustermap {

    const int MASTER_TCP_PORT = 8881;
    const int MASTER_UDP_PORT = 9991;
    const int SLAVE_TCP_PORT = 8882;
    const int SLAVE_UDP_PORT = 9992;

    class ClustermapServerTool
    {
        public:
            bool startMasterServer();
            bool finishMasterServer();
            bool startSlaveServer();
            bool finishSlaveServer();
    };

    static void* MasterServer_thr_func(void *)
    {
        int ret = system("../src/clustermap 8881 9991 ./cm.xml 1>masterserver_stdout 2>masterserver_stderr");
        if (ret != 0) {
            return false;
        }
    }

    bool ClustermapServerTool::startMasterServer()
    {
        pthread_t tid;
        pthread_create(&tid, NULL, MasterServer_thr_func, NULL);

        sleep(2);
        FILE *fp = fopen("masterserver_stdout", "r");
        if (!fp) {
            return false;
        }
        char str[100];
        int count = 0;
        while (fgets(str, 100, fp)) {
            if (strstr(str, "ok")) {
                count++;
            }
        }
        fclose(fp);
        if (count == 3)
            return true;
        else
            return false;
    }

    bool ClustermapServerTool::finishMasterServer()
    {
        int ret = system("killall clustermap");
        if (ret != 0) {
            return false;
        }
        return true;
    }

    static void* SlaveServer_thr_func(void *)
    {
        int ret = system("../src/clustermap 8882 9992 ./cm.xml tcp:127.0.0.1:8881 1>slaveserver_stdout 2>slaveserver_stderr");
        if (ret != 0) {
            return false;
        }
    }

    bool ClustermapServerTool::startSlaveServer()
    {
        pthread_t tid;
        pthread_create(&tid, NULL, SlaveServer_thr_func, NULL);

        sleep(2);
        FILE *fp = fopen("slaveserver_stdout", "r");
        if (!fp) {
            return false;
        }
        char str[100];
        int count = 0;
        while (fgets(str, 100, fp)) {
            if (strstr(str, "ok")) {
                count++;
            }
        }
        fclose(fp);
        if (count == 3)
            return true;
        else
            return false;
    }

    bool ClustermapServerTool::finishSlaveServer()
    {
        int ret = system("killall clustermap");
        if (ret != 0) {
            return false;
        }
        return true;
    }

}
