#include "framework/Command.h"
#include "framework/Server.h"
#include "config.h"
#include "buildinfo.h"
#include "util/ThreadLock.h"
#include "util/ScopedLock.h"
#include "util/Log.h"
#include "util/fileutil.h"
#include <alog/Configurator.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#define pidFileTMPL "/tmp/.%s_%s_pidfile"

FRAMEWORK_BEGIN;

typedef enum {
    cmd_start,
    cmd_stop,
    cmd_restart,
    cmd_unknown
} cmd_type_t;

class Args {
    public:
        Args(int argc, char *argv[]);
        ~Args() { }
    public:
        const char *getLogConfPath() const { return _logPath; }
        const char *getConfPath() const { return _confPath; }
        bool isForHelp() const { return _help; }
        bool isForShortVersion() const { return _verShort; }
        bool isForLongVersion() const { return _verLong; }
        bool asDaemon() const { return _daemon; }
        cmd_type_t getCommandType() const { return _cmd; }
    private:
        char _logPath[PATH_MAX + 1];
        char _confPath[PATH_MAX + 1];
        bool _verShort;
        bool _verLong;
        bool _help;
        bool _daemon;
        cmd_type_t _cmd;
};

Args::Args(int argc, char *argv[]) 
    : _verShort(false), 
    _verLong(false), 
    _help(false), 
    _daemon(false), 
    _cmd(cmd_start) 
{
    int c = 0;
    _logPath[0] = '\0';
    _confPath[0] = '\0';
    if (argv == NULL) {
        return;
    }
    while ((c = getopt(argc, argv, "k:c:dl:vVh")) != -1) {
        switch(c) {
            case 'c':
                strcpy(_confPath, optarg); break;
            case 'l':
                strcpy(_logPath, optarg); break;
            case 'k':
                if (strcmp(optarg, "restart") == 0) {
                    _cmd = cmd_restart;
                }
                else if (strcmp(optarg, "start") == 0) {
                    _cmd = cmd_start;
                }
                else if (strcmp(optarg, "stop") == 0) {
                    _cmd = cmd_stop;
                }
                else {
                    _cmd = cmd_unknown;
                }
                break;
            case 'd':
                _daemon = true; break;
            case 'v':
                _verShort = true; break;
            case 'V':
                _verLong = true; break;
            case 'h':
                _help = true; break;
            case '?':
                if (optopt != 'c' && optopt != 'l' && isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", 
                            optopt);
                }
                break;
            default:
                abort();
        }
    }
    if (_daemon) {
        //TODO: convert to abs path
    }
}

FRAMEWORK_END;

FRAMEWORK_BEGIN;
pid_t getPidFromFile(const char *pidfile) 
{
    long p = 0;
    pid_t pid;
    FILE *file = NULL;
    /* make sure this program isn't already running. */
    file = fopen (pidfile, "r");
    if (!file) {
        return -1;
    }
    if (fscanf(file, "%ld", &p) == 1 && (pid = p) && (getpgid (pid) > -1)) {
        fclose (file);
        return pid;
    }
    fclose (file);
    return 0;
}

int updatePidFile (const char *pidfile) 
{
    long p = 0;
    pid_t pid;
    mode_t prev_umask;
    FILE *file = NULL;
    /* make sure this program isn't already running. */
    file = fopen (pidfile, "r");
    if (file) {
        if (fscanf(file, "%ld", &p) == 1 && (pid = p) && 
                (getpgid (pid) > -1)) 
        {
            fclose (file);
        }
    }
    /* write the pid of this process to the pidfile */
    prev_umask = umask (0112);
    unlink(pidfile);
    file = fopen (pidfile, "w");
    if (!file) {
        fprintf(stderr, "Error writing pidfile '%s' -- %s\n", 
                pidfile, strerror (errno));
        return -1;
    }
    fprintf (file, "%ld\n", (long) getpid());
    fclose (file);
    umask (prev_umask);
    return 0;
}
FRAMEWORK_END;


FRAMEWORK_BEGIN;

CommandLine::CommandLine(const char *name) 
    : _name(NULL), 
    _server(NULL), 
    _next(NULL) 
{
    if (name) {
        _name = strdup(name);
    } 
    else {
        _name = strdup("unknown-server");
    }
}

CommandLine::~CommandLine() 
{
    if (_name) {
        ::free(_name);
    }
    if (_server) {
        delete _server;
    }
}

static UTIL::Mutex G_CMD_LOCK;
static CommandLine *G_CMD_LIST = NULL;

int CommandLine::run(int argc, char *argv[]) 
{
    int32_t ret = 0;
    int32_t err = 0;
    sigset_t oldMask;
    bool needWait = false;
    char pidFile[PATH_MAX + 1] = {0};
    char confName[PATH_MAX + 1] = {0};
    pid_t curPid = 0;
    Args args(argc, argv);
    char *pconf = get_realpath(args.getConfPath(), confName);
    if (_server) {
        fprintf(stderr, "%s is running.\n", _name);
        return -1;
    }
    if (args.isForHelp()) {
        help();
        return 0;
    } 
    else if (args.isForLongVersion()) {
        showLongVersion();
        return 0;
    } 
    else if (args.isForShortVersion()) {
        showShortVersion();
        return 0;
    }
    sigemptyset(&_mask);
    sigaddset(&_mask, SIGINT);
    sigaddset(&_mask, SIGTERM);
    sigaddset(&_mask, SIGUSR1);
    sigaddset(&_mask, SIGUSR2);
    sigaddset(&_mask, SIGPIPE);
    if ((err=pthread_sigmask(SIG_BLOCK, &_mask, &oldMask))!=0) {
        fprintf(stderr, "Set sigmask error.\n", _name);
        return -1;
    }
    //    register_sig();
    for (char *pTmp=pconf;*pTmp;pTmp++) {
        if (*pTmp == '/') {
            *pTmp='_';
        }
    }
    snprintf(pidFile, strlen(_name)+strlen(pidFileTMPL)+strlen(pconf)+1, 
            pidFileTMPL, _name, pconf);
    curPid = getPidFromFile(pidFile);
    if (args.getCommandType() == cmd_unknown
            || strlen(args.getConfPath()) == 0) 
    {
        help(-1);
        return -1;
    }
    else if (args.getCommandType() == cmd_stop) {
        if (curPid > 0) {
            if (kill(curPid, SIGTERM)!=0) {
                fprintf(stderr, "kill process error, pid = %ld\n", curPid);
            }
        }
        else {
            fprintf(stderr, "process not running.\n");
        }
        return -1;
    }
    else if (args.getCommandType() == cmd_restart) {
        if (curPid > 0) {
            if (kill(curPid, SIGTERM)!=0) {
                fprintf(stderr, "kill process error, pid = %ld\n", curPid);
                return -1;
            }
            needWait = true;
        }
        else {
            fprintf(stderr, "process not running.\n");
        }
    }
    else if (args.getCommandType() == cmd_start) {
        if (curPid > 0) {
            fprintf(stderr, "process already running."
                    "stop it first. pid=%ld, pidfile=%s\n", curPid, pidFile);
            return -1;
        }
    }
    _server = makeServer();
    if (unlikely(!_server)) {
        alog::Logger::shutdown();
        fprintf(stderr, "create server failed.\n");
    }
    if (args.asDaemon()) {
        if (daemon(1, 1) == -1) {
            fprintf(stderr, "set self process as daemon failed.\n");
        }
    }
    if ((args.getCommandType()==cmd_restart)&&(needWait)) {
        int errnum = 0;
        while (getPidFromFile(pidFile) >= 0) {
            if (errnum > 3000) {
                fprintf(stderr, "shutdown previous server failed: too slow\n");
                if (_server) {
                    delete _server;
                }
                alog::Logger::shutdown();
                return -1;
            }
            usleep(20);
            errnum++;
        }
    }
    if (updatePidFile(pidFile) < 0) {
        fprintf(stderr, "write pid to file %s error\n", pidFile);
        delete _server;
        _server = NULL;
        alog::Logger::shutdown();
        return -1;
    }
    if (strlen(args.getLogConfPath()) > 0) {
        try {
            alog::Configurator::configureLogger(args.getLogConfPath());
        } catch(std::exception &e) {
            fprintf(stderr, "config from '%s' failed, using default root.\n",
                    args.getLogConfPath());
            alog::Configurator::configureRootLogger();
        }
    } else {
        alog::Configurator::configureRootLogger();
    }
    ret = _server->start(args.getConfPath());
    if (ret != KS_SUCCESS) {
        fprintf(stderr, "startting server by `%s' failed(%d).\n", 
                args.getConfPath(), ret);
        delete _server;
        _server = NULL;
        alog::Logger::shutdown();
        unlink(pidFile);
        return -2;
    }
    MUTEX_LOCK(G_CMD_LOCK);
    _next = G_CMD_LIST;
    G_CMD_LIST = this;
    MUTEX_UNLOCK(G_CMD_LOCK);
    waitSig();
    _server->wait();
    delete _server;
    _server = NULL;
    alog::Logger::shutdown();
    unlink(pidFile);
    MUTEX_LOCK(G_CMD_LOCK);
    if (G_CMD_LIST == this) {
        G_CMD_LIST = _next;
    } 
    else {
        for (CommandLine *prev = G_CMD_LIST; prev; prev = prev->_next) {
            if (prev->_next == this) {
                prev->_next = _next;
                break;
            }
        }
    }
    MUTEX_UNLOCK(G_CMD_LOCK);
    return 0;
}

void CommandLine::terminate() 
{
    if (_server) {
        _server->stop();
    }
}

void CommandLine::help(int type) 
{
    FILE * fp = NULL;
    if (type == 0) {
        fp = stdout;
    }
    else {
        fp = stderr;
    }
    fprintf(fp, "Usage: %s -c <server_confPath> -k start|stop|restart [-l <log_confPath>] [-d]\n",
            _name);
    fprintf(fp, "             [-v] [-V] [-h]\n");
    fprintf(fp, "Options:\n");
    fprintf(fp, "  -c <server_confPath>    : specify an alternate ServerConfigFile\n");
    fprintf(fp, "  -l <log_confPath>       : specify an alternate log serup conf file\n");
    fprintf(fp, "  -k start|stop|restart    : specify an alternate server operator\n");
    fprintf(fp, "  -d                       : set self process as daemon\n");
    fprintf(fp, "  -v                       : show version number\n");
    fprintf(fp, "  -V                       : show compile settings\n");
    fprintf(fp, "  -h                       : list available command line options (this page)\n");
    fprintf(fp, "Example:\n");
    fprintf(fp, "  %s -c server.cfg -k restart -l log.cfg -d\n", _name);
}

void CommandLine::showShortVersion() 
{
    fprintf(stdout, "%s\n", VERSION);
}

void CommandLine::showLongVersion() 
{
    fprintf(stdout, "Server version : %s/%s %s (Linux)\n", 
            _name, VERSION, RELEASE);
    fprintf(stdout, "Server built   : %s %s by %s\n",
            __DATE__, __TIME__, USER);
    fprintf(stdout, "Server root    : %s\n", PREFIX);
    fprintf(stdout, "Architecture   : %ld-bit\n", 8 * (long)sizeof(void*));
    fprintf(stdout, "Bug Report     : <%s>\n", PACKAGE_BUGREPORT);
    fprintf(stdout, "Server compiled with....\n");
    fprintf(stdout, "  CFLAGS=%s\n", CFLAGS);
    fprintf(stdout, "  CXXFLAGS=%s\n", CXXFLAGS);
}

static void handleServer(int sig) 
{
    MUTEX_LOCK(G_CMD_LOCK);
    if (sig == SIGUSR1 || sig == SIGUSR2 || 
            sig == SIGINT || sig == SIGTERM) 
    {
        for(CommandLine *p = G_CMD_LIST; p; p = p->_next) {
            p->terminate();
        }
    }
    MUTEX_UNLOCK(G_CMD_LOCK);
}

void CommandLine::registerSig() 
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGUSR1, handleServer);
    signal(SIGUSR2, handleServer);
    signal(SIGINT, handleServer);
    signal(SIGTERM, handleServer);
}

void CommandLine::waitSig() 
{
    int err = 0;
    int signo = 0;
    while (1) {
        err = sigwait(&_mask, &signo);
        if (err != 0) {
            continue;
        }    
        switch (signo) {
            case SIGUSR1:
            case SIGUSR2:
            case SIGINT:
            case SIGTERM:
                MUTEX_LOCK(G_CMD_LOCK);
                for (CommandLine *p = G_CMD_LIST; p; p = p->_next) {
                    p->terminate();
                }
                MUTEX_UNLOCK(G_CMD_LOCK);
                return;
            default:
                continue;
        }
    }//while
}

FRAMEWORK_END;

