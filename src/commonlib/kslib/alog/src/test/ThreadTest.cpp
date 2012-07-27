#include "../include/Logger.h"
#include "../include/Appender.h"
#include "../include/Configurator.h"
#include <fstream>
#include <string>
#include <assert.h>

#define THREAD_NUM 200
#define LOG_NUM 10000

using namespace std;
using namespace alog;

void *thread(void *arg)
{
    Logger* logger = Logger::getLogger("ThreadTest");
    for (int i = 0 ; i < LOG_NUM; i++)
    {
        logger->log(LOG_LEVEL_ERROR,"|threadtest1threadtest2threadtest3threadtest4threadtest5|");
    }
    return NULL;
}

int getFileLineCount(const char * filename)
{
    std::ifstream logfile(filename);
    char buffer[256];
    int lineCount = 0;
    std::string::size_type length;
    while (logfile.getline(buffer, sizeof(buffer)))
    {
        std::string lineStr = buffer;
        length = lineStr.find("|threadtest1threadtest2threadtest3threadtest4threadtest5|");
        assert(length != std::string::npos);
        lineCount++;
    }
    return lineCount;
}

int main(int argc, char *argv[])
{
    unlink("thread.log");
    Logger::getLogger("ThreadTest")->addAppender(FileAppender::getAppender("thread.log"));

    pthread_t tid[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM ; i++)
        pthread_create(&tid[i],NULL,thread,&i);
    for (int i = 0; i < THREAD_NUM; i++)
        pthread_join(tid[i],NULL);

    int lineCount = getFileLineCount("thread.log");
    assert(lineCount == THREAD_NUM * LOG_NUM);
    Logger::shutdown();
    unlink("thread.log");
    printf("Thread safe!\n");
}

