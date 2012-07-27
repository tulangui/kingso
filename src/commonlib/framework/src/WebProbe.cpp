#include "framework/WebProbe.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFERSIZE 10240

FRAMEWORK_BEGIN;

WebProbe::WebProbe()
    :_terminated(false),
    _probeQueue(NULL),
    _pid(0) {
    }

WebProbe::~WebProbe()
{
    terminate();
    if (_probeQueue) {
        _probeQueue->terminate();
    }
    if (_pid != 0) {
        pthread_join(_pid, NULL);
    }
    if (_probeQueue) {
        delete _probeQueue;
    }
}

void WebProbe::terminate()
{
    _terminated = true;
}

bool WebProbe::isProbeRequest(const char *uri)
{
    if (!uri) {
        return false;
    }
    return true;
}

int32_t WebProbe::initialize(const char *httproot)
{
    if (!httproot) {
        return KS_EFAILED;
    }
    _httpRoot = httproot;
    _probeQueue = new (std::nothrow) TaskQueue;
    if (pthread_create(&_pid, NULL, threadHook, this)) {
        return KS_EFAILED;
    }
    return KS_SUCCESS;

}

void *WebProbe::threadHook(void *args)
{
    if (args == NULL) {
        return NULL;
    }
    WebProbe *webprobe = (WebProbe *)args;
    return webprobe->run();
}

void *WebProbe::run()
{
    int32_t ret = 0;
    Session *session = NULL;
    while (1)
    {
        if (_terminated) {
            return NULL;
        }
        ret = 0;
        session = NULL;
        ret = _probeQueue->dequeue(&session);
        if (unlikely(ret == EAGAIN)) {
            continue;
        }
        if (unlikely(ret == EOF)) {
            continue;
        }
        if (unlikely(ret != KS_SUCCESS)) {
            continue;
        }
        if (unlikely(!session)) {
            continue;
        }
        std::string filename = _httpRoot;
        filename += session->getQuery().getOrigQueryData();
        int fd,size;
        struct stat statInfo;
        if (stat(filename.c_str(), &statInfo ) < 0) {
            session->setStatus(FRAMEWORK::ss_notexist);
            session->response(false);
            continue;
        }

        char *pBuffer = (char *)malloc(BUFFERSIZE);
        fd = open(filename.c_str(), O_RDONLY);
        if (fd <= 0) {
            free(pBuffer);
            session->setStatus(FRAMEWORK::ss_notexist);
            session->response(false);
            continue;
        }

        size = read(fd, pBuffer, BUFFERSIZE-1);
        close(fd);
        if (size <= 0) {
            free(pBuffer);
            session->setStatus(FRAMEWORK::ss_error);
            session->response(false);
            continue;
        }
        pBuffer[size] = 0;
        session->setResponseData(pBuffer, strlen(pBuffer)+1);
        session->response(false);

    }
}

FRAMEWORK_END;
