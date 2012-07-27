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
 * $Id: WebClient.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include "CMTCPClient.h"

using namespace clustermap;

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        printf("%s blender_protocol:blender_ip:blender_port sendMaxNum sendFrequnce(ms)\n", argv[0]);
        return -1;
    }

    CMTCPClient cClient;

    char* spec = "tcp:127.0.0.1:8080";
    int32_t sendFre = 3000, sendmax = 100;

    if(argc > 1) spec = argv[1];
    if(argc > 2) sendmax = atol(argv[2]);
    if(argc > 3) sendFre = atol(argv[3]) * 1000;

    std::string str = spec;
    str.replace(0, 4, std::string("tcp"), 0, 3);
    if(cClient.setServerAddr(str.c_str()))
        return -1;
    if(cClient.start())
        return -1;
    FILE* fpLog = fopen("webClient.log", "w");
    if(fpLog == NULL)
        return -1;

    uint32_t word = 1;
    char buffer[4096], *repBuf;

    while(1)
    {
        int32_t buflen = sprintf(buffer, "%d---%s", word, spec);
        if(cClient.send(buffer, buflen) == 0) {
            if(cClient.recv(repBuf, buflen) == 0)
                fprintf(fpLog, "sucess:no=%d, send:%s\n       recv:%s\n\n", word, buffer, repBuf+1);
            else
                fprintf(fpLog, "fail:no=%d, send:%s\n\n\n\n", word, buffer);
        }
        else
            fprintf(fpLog, "fail:no=%d, send:%s failed\n\n\n\n", word, buffer);
        word++;
        fflush(fpLog);

        if((int32_t)word > sendmax)
            break;
        if(sendFre) usleep(sendFre);
    }
    fclose(fpLog);

    return 0;
}
