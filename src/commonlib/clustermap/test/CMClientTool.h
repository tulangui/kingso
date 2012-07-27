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

namespace clustermap {

    class BlenderMainTool
    {
        public:
            bool startCMClient();
    };

    static void* CMClient_thr_func(void *)
    {
        int ret = system("../src/blender http:10.232.42.17:6611 127.0.0.1:8881:9991,127.0.0.1:8802:9902 null 1>cmclient_stdout 2>cmclient_stderr");
        if (ret != 0) {
            return false;
        }
    }


    bool NodeinfoTool::hasClusterName(const char *clustername, char *state)
    {
        for (int i = 0; i < result.clustercount; i++) {
            if (strcmp(clustername, result.clusters[i].Clustername) == 0) {
                return true;
            }
        }
        return false;
    }

    bool NodeinfoTool::hasNodeSpec(const char *nodespec, char *state)
    {
        return false;
    }

    bool NodeinfoTool::NodeinfoWall()
    {
        int ret = system("../src/nodeinfo tcp:127.0.0.1:8881 tcp:127.0.0.1:8882 -w all 1>nodeinfo_stdout 2>nodeinfo_stderr");
        if (ret != 0) {
            return false;
        }
        FILE *fp = fopen("nodeinfo_stdout", "r");
        char str[100];
        result.clustercount = -1;
        while (fgets(str, 100, fp)) {
            if (isClusterStr(str)) {
                result.clustercount++;
                char *p = strchr(str, ' ');
                *p = '\0';
                strcpy(result.clusters[result.clustercount].Clustername, str);
                result.clusters[result.clustercount].nodecount = 0;
                continue;
            } else if (isNodeStr(str)) {
                char *p = str;
                p = strchr(p, ':');
                p = strchr(p+1, ':');
                p = strchr(p+1, ':');
                *p = '\0';
                int nodecount = result.clusters[result.clustercount].nodecount;
                strcpy(result.clusters[result.clustercount].NodeSpec[nodecount], str + 1);
                result.clusters[result.clustercount].nodecount++;
                continue;
            }
        }
        result.clustercount++;
        fclose(fp);
        /*
           for (int i = 0; i < result.clustercount; i++) {
           printf("i = %d Clustername = %s\n", i, result.clusters[i].Clustername);
           for (int j = 0; j < result.clusters[i].nodecount; j++) {
           printf("j = %d NodeSpec = %s\n", j, result.clusters[i].NodeSpec[j]);
           }
           }
         */
        return true;
    }

    bool NodeinfoTool::isClusterStr(const char *str)
    {
        if (strstr(str, "GREEN") || strstr(str, "YELLOW") || strstr(str, "RED")) {
            return true;
        }
        return false;
    }

    bool NodeinfoTool::isNodeStr(const char *str)
    {
        if (strstr(str, "http:") || strstr(str, "tcp:") || strstr(str, "udp:")) {
            return true;
        }
        return false;
    }

}
