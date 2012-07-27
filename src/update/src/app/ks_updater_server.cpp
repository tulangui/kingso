#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <malloc.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "util/fileutil.h"
#include "UpdaterServer.h"

static void pidfilepath(char buf[], const char* pro_name, const char* cfg_path)
{
    char* ptr = 0;
    char cgfbuf[PATH_MAX+1];
    char* real_cfg_path = get_realpath(cfg_path, cgfbuf);
    if(!real_cfg_path){
        real_cfg_path = (char*)cfg_path;
    }
    snprintf(buf, PATH_MAX+1, "/tmp/%s_%s", pro_name, real_cfg_path);
    for(ptr=buf+5; *ptr!=0; ptr++){
        if(*ptr=='/'){
            *ptr = '_';
        }
    }
}

static pid_t getpidfile(const char* pro_name, const char* cfg_path)
{
    char strbuf[PATH_MAX+1];
    FILE* file = 0;
    pidfilepath(strbuf, pro_name, cfg_path);
    if(access(strbuf, F_OK)){
        return -1;
    }
    file = fopen(strbuf, "r");
    if(!file){
        return -1;
    }
    strbuf[0] = 0;
    if(fgets(strbuf, PATH_MAX+1, file)==0){
        fclose(file);
        return -1;
    }
    fclose(file);
    return atoi(strbuf);
}

static int setpidfile(const char* pro_name, const char* cfg_path)
{
    char strbuf[PATH_MAX+1];
    FILE* file = 0;
    pidfilepath(strbuf, pro_name, cfg_path);
    file = fopen(strbuf, "w");
    if(!file){
        return -1;
    }
    fprintf(file, "%d\n", getpid());
    fclose(file);
    return 0;
}

static void delpidfile(const char* pro_name, const char* cfg_path)
{
    char strbuf[PATH_MAX+1];
    pidfilepath(strbuf, pro_name, cfg_path);
    unlink(strbuf);
}

static void waitsig(sigset_t* mask) 
{
    int ret = 0;
    int signo = 0;

    while(1){
        ret = sigwait(mask, &signo);
        if(ret!=0){
            continue;
        }    
        switch(signo){
            case SIGUSR1:
            case SIGUSR2:
            case SIGINT:
            case SIGTERM:
                return;
            default:
                continue;
        }   
    }//while
}

static void usage(const char* pro_name)
{
    printf("Usage: %s -c <server_confPath> -k start|stop|restart [-l <log_confPath>] [-d] [-h]\n", pro_name?pro_name:"unknow");
    printf("Options:\n");
    printf("  -c <server_confPath>  : specify an alternate ServerConfigFile\n");
    printf("  -l <log_confPath>     : specify an alternate log serup conf file\n");
    printf("  -k start|stop|restart : specify an alternate server operator\n");
    printf("  -d                    : set self process as daemon\n");
    printf("  -h                    : list available command line options (this page)\n");
    printf("Example:\n");
    printf("  %s -c server.cfg -k restart -l log.cfg -d\n", pro_name?pro_name:"unknow");
}

int main(int argc, char* argv[])
{
    const char* pro_name = 0;
    const char* cfg_path = 0;
    const char* log_path = 0;
    const char* command = "start";
    int isdaemon = 0;
    int arg = 0;
    int err = 0;
    sigset_t new_mask, old_mask;
    unsigned int now = time(0);
    int slp_time = (rand_r(&now)%1000+1)*1000;
    char* last_line = 0;

    last_line = strrchr(argv[0], '/');
    if(last_line){
        pro_name = last_line+1;
    }   
    else{
        pro_name = argv[0];
    }
    while((arg=getopt(argc, argv, "c:k:l:dh"))!=-1){
        switch(arg){
            case 'c' :
                cfg_path = optarg;
                break;
            case 'k' :
                command = optarg;
                break;
            case 'l' :
                log_path = optarg;
                break;
            case 'd' :
                isdaemon = 1;
                break;
            case 'h' :
            case '?' :
            default :
                usage(pro_name);
                return 0;
        }
    }

    if(!pro_name || !cfg_path || !command){
        usage(pro_name);
        return 0;
    }

    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGINT);
    sigaddset(&new_mask, SIGTERM);
    sigaddset(&new_mask, SIGUSR1);
    sigaddset(&new_mask, SIGUSR2);
    sigaddset(&new_mask, SIGPIPE);
    if((err=pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask))) {
        fprintf(stderr, "set sigmask error[%d], return!\n", err);
        return -1; 
    }

    usleep(slp_time);
    if(strcasecmp(command, "start")==0){
        pid_t pid = getpidfile(pro_name, cfg_path);
        if(pid>0){
            if(kill(pid, 0)==0){
                fprintf(stderr, "%s running, exit!\n", pro_name);
                return 0;
            }   
            else{
                delpidfile(pro_name, cfg_path);
            }   
        }   
    }   
    else if(strcasecmp(command, "stop")==0){
        pid_t pid = getpidfile(pro_name, cfg_path);
        if(pid>0){
            if(kill(pid, 0)==0){
                kill(pid, SIGTERM);
            }   
            else{
                fprintf(stderr, "%s not running!\n", pro_name);
            }   
            delpidfile(pro_name, cfg_path);
        }   
        else{
            fprintf(stderr, "%s not running!\n", pro_name);
        }   
        return 0;
    }   
    else if(strcasecmp(command, "restart")==0){
        pid_t pid = getpidfile(pro_name, cfg_path);
        if(pid>0){
            if(kill(pid, 0)){
                fprintf(stderr, "%s not running!\n", pro_name);
                delpidfile(pro_name, cfg_path);
            }   
            else{
                kill(pid, SIGTERM);
                while(getpidfile(pro_name, cfg_path)>0){
                    usleep(slp_time);
                }   
                delpidfile(pro_name, cfg_path);
            }   
        }   
        else{
            fprintf(stderr, "%s not running!\n", pro_name);
        }   
    }
    else{
        usage(pro_name);
        return 0;
    }

    if(isdaemon){
        daemon(1, 1);
    }

    update::UpdaterServer server;

    setpidfile(pro_name, cfg_path);
    if(server.init(cfg_path, log_path) < 0) {
        delpidfile(pro_name, cfg_path);
        fprintf(stderr, "init cfg: %s failed", cfg_path);
        return -1;
    }
    if(server.start() < 0) {
        delpidfile(pro_name, cfg_path);
        fprintf(stderr, "start error!\n");
        return -1;
    }

    waitsig(&new_mask);
    server.stop();
    delpidfile(pro_name, cfg_path);
    return 0;
}

