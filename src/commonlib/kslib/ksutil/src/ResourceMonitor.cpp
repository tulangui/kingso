#include "util/ResourceMonitor.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

UTIL_BEGIN;

ResourceManager::ResourceManager() {
	memset(_proxy, 0, sizeof(_proxy));
}

ResourceManager::~ResourceManager() {
	for(int i = 0; i < 256; i++) {
		if(_proxy[i]) delete _proxy[i];
	}
}

bool ResourceManager::registerProxy(char c, ResourceInterface * r) {
	if(!r || _proxy[c]) return false;
	_proxy[c] = r;
	return true; 
}

ResourceInterface * ResourceManager::cancel(char c) {
	ResourceInterface * r = _proxy[c];
	_proxy[c] = NULL;
	return r;
}

ResourceMonitor::ResourceMonitor(const ResourceManager & mgr) 
: _mgr(mgr), _running(false) {
	_pipe_path[0] = '\0';
}

ResourceMonitor::~ResourceMonitor() {
}

bool ResourceMonitor::handleEvent(char c) {
	ResourceInterface * r = _mgr.get(c);
	return r ? r->reload() : false;
}

static void * thread_hook(void * arg) {
	ResourceMonitor * monitor = (ResourceMonitor*)arg;
	if(monitor) monitor->monitor();
	return (void*)0;
}

bool ResourceMonitor::start(const char * path) {
	if(_running) return true;
	if(!path || strlen(path) > PATH_MAX) return false;
	strcpy(_pipe_path, path);
	_running = true;
	if(pthread_create(&_tid, NULL, thread_hook, this) != 0) {
		fprintf(stderr, "pthread_create error.\n");
		return false;
	}
	return true;
}

void ResourceMonitor::wait() {
	pthread_join(_tid, NULL);
}

bool ResourceMonitor::monitor() {
	int fd, n;
	char c;
	umask(0);
	if((mkfifo(_pipe_path, 
					(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) && 
			(errno != EEXIST)) {
		return false;
	}
	fd = open(_pipe_path, (O_RDONLY|O_NONBLOCK));
	if(fd < 0) return false;
	while(_running) {
		if((n=read(fd, &c, 1)) < 0 && errno != EAGAIN) break;
		if(!_running) break;
		if(n == 1) handleEvent(c);
	}
	close(fd);
	return true;
}

bool ResourceMonitor::terminate() {
	int fd;
	char c = '\0';
	_running = false;
	fd = open(_pipe_path, O_WRONLY);
	if(fd == -1) return false;
	if(write(fd, &c, 1) != 1) {
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

UTIL_END;
