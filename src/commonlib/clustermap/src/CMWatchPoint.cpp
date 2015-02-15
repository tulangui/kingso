/** \file
 *******************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 11573 $
 *
 * $LastChangedDate: 2012-04-25 15:55:14 +0800 (Wed, 25 Apr 2012) $
 *
 * $Id: CMWatchPoint.cpp 11573 2012-04-25 07:55:14Z pujian $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <stdlib.h>
#include <unistd.h>
#include "CMWatchPoint.h"

namespace clustermap {

struct cpu_info {
    long user;
    long system;
    long nice;
    long idle;
    long iowait;
    long irq;
    long softirq;

    int getCpuResource() {
        char cpu[21];
        char text[201];
        FILE *fp = fopen("/proc/stat", "r");
        if (fp == NULL) {
            printf("can't open file /proc/stat\n");
            return -1;
        }
        if (fgets(text, 200, fp) && strstr(text, "cpu")) {
            sscanf(text, "%s %ld %ld %ld %ld %ld %ld %ld", cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq);
        }
        fclose(fp);
        return 0;
    }
};

bool ResourceWatchPoint::isOK() {
    uint32_t curStatus = getCurrentResource();
    if(curStatus >= _limit) {
        _overload_count++;
        if(_overload_count >= _overload_count_limit) return false;
    } else {
        _overload_count = 0;
    }
    return true;
}

uint32_t CPUWatchPoint::getCurrentResource() {
    //TODO: read /proc/stat, parse line "cpu ...".
    cpu_info prev, cur;
    if(prev.getCpuResource() < 0) {
        return 0;
    }
    usleep(100000);
    if(cur.getCpuResource() < 0) {
        return 0;
    }

    int32_t total_prev = prev.user + prev.nice + prev.system + prev.idle + prev.iowait + prev.irq + prev.softirq;
    int32_t total_cur = cur.user + cur.nice + cur.system + cur.idle + cur.iowait + cur.irq + cur.softirq;
    int32_t total = total_cur - total_prev;
    int32_t idle = cur.idle - prev.idle;
    double cpu_busy = 100.0 - (double)idle / (double)total * 100.0;
    int32_t io  = cur.iowait - prev.iowait;
    double iowait = (double)io / (double)total * 100.0;

    CPUWatchResult res;
    res.sVal.cpu_busy = (uint16_t)cpu_busy;
    res.sVal.io_wait  = (uint16_t)iowait;
    return res.value;
}

uint32_t SysloadWatchPoint::getCurrentResource() {
    double sys_load;
    if(getloadavg(&sys_load, 1) == -1) return 0;
    return (uint32_t)(sys_load + 0.9999);
}

}

