#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>

extern int listening(const unsigned short port)
{
    int fd = -1;
    int reuse = 1;
    struct sockaddr_in addr_in;
    struct sockaddr* addr = (struct sockaddr*)(&addr_in);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd<0){
        return -1;
    }

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))){
        close(fd);
        return -1;
    }

    bzero(&addr_in, sizeof(struct sockaddr_in));
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_port = htons(port);
    addr_in.sin_family = AF_INET;
 
    if(bind(fd, addr, sizeof(struct sockaddr_in))){
        close(fd);
        return -1;
    }

    if(listen(fd, 100)){
        close(fd);
        return -1;
    }

    return fd;
}


extern int accept_wait(int socket, struct sockaddr* address, socklen_t* address_len, int* running)
{
    int vrunning = 1;
    int noblock = 1;
    struct timeval timeout = {0, 1000};
    fd_set read_set;
    fd_set error_set;
    int ret = 0;

    if(!running){ 
        running = &vrunning;
    }

    if(ioctl(socket, FIONBIO, &noblock)){
        return -1;
    }

    while(*running!=0){
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        FD_ZERO(&read_set);
        FD_ZERO(&error_set);
        FD_SET(socket, &read_set);
        FD_SET(socket, &error_set);
        ret = select(socket+1, &read_set, 0, &error_set, &timeout);
        if(ret<0){
            if(errno==EINTR){
                continue;
            }
            else{
                noblock = 0;
                if(ioctl(socket, FIONBIO, &noblock)){
                    return -1;
                }
                return -1;
            }
        }
        else if(ret==0){
            continue;
        }
        if(FD_ISSET(socket, &error_set)){
            noblock = 0;
            if(ioctl(socket, FIONBIO, &noblock)){
                return -1;
            }
            return -1;
        }
        ret = accept(socket, address, address_len);
        if(ret<0){
            if(errno==EAGAIN || errno==EINTR){
                continue;
            }
            else{
                noblock = 0;
                if(ioctl(socket, FIONBIO, &noblock)){
                    return -1;
                }
                return -1;
            }
        }
        else{
            noblock = 0;
            if(ioctl(socket, FIONBIO, &noblock)){
                return -1;
            }
            return ret;
        }
    }

    noblock = 0;
    if(ioctl(socket, FIONBIO, &noblock)){
        return -1;
    }
    return -1;
}

extern int connect_wait(int socket, const struct sockaddr* address, socklen_t address_len, int* running)
{
    int vrunning = 1;
    int noblock = 1;

    if(!running){ 
        running = &vrunning;
    }

    if(ioctl(socket, FIONBIO, &noblock)){
        return -1;
    }

    while(*running!=0){
        if(connect(socket, address, address_len)==0){
            noblock = 0;
            if(ioctl(socket, FIONBIO, &noblock)){
                return -1;
            }
            return 0;
        }
        usleep(1000);
    }

    noblock = 0;
    if(ioctl(socket, FIONBIO, &noblock)){
        return -1;
    }
    return -1;
}

extern int connect_times(int socket, const struct sockaddr* address, socklen_t address_len, int times)
{
    int noblock = 1;
    int i = 0;

    if(ioctl(socket, FIONBIO, &noblock)){
        return -1;
    }

    for(; i<times; i++){
        if(connect(socket, address, address_len)==0){
            noblock = 0;
            if(ioctl(socket, FIONBIO, &noblock)){
                return -1;
            }
            return 0;
        }
        usleep(1000);
    }

    noblock = 0;
    if(ioctl(socket, FIONBIO, &noblock)){
        return -1;
    }
    return -1;   
}

extern ssize_t read_wait(int fildes, void* buf, size_t nbyte, int* running)
{
    int vrunning = 1;
    int noblock = 1;
    size_t done = 0;
    ssize_t ret = 0;
    struct timeval timeout = {0, 1000};
    fd_set read_set;
    fd_set error_set;

    if(nbyte==0){
        return 0;
    }

    if(!running){
        running = &vrunning;
    }

    if(ioctl(fildes, FIONBIO, &noblock)){
        return -1;
    }

    while(*running!=0 && done<nbyte){
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        FD_ZERO(&read_set);
        FD_ZERO(&error_set);
        FD_SET(fildes, &read_set);
        FD_SET(fildes, &error_set);
        ret = select(fildes+1, &read_set, 0, &error_set, &timeout);
        if(ret<0){
            if(errno==EINTR){
                continue;
            }
            else{
                noblock = 0;
                if(ioctl(fildes, FIONBIO, &noblock)){
                    return -1;
                }
                return -1;
            }
        }
        else if(ret==0){
            continue;
        }
        if(FD_ISSET(fildes, &error_set)){
            noblock = 0;
            if(ioctl(fildes, FIONBIO, &noblock)){
                return -1;
            }
            return -1;
        }
        ret = read(fildes, (char*)buf+done, nbyte-done);
        if(ret>0){
            done += ret;
        }   
        else if(ret<0 && errno!=EINTR && errno!=EAGAIN){
            noblock = 0;
            if(ioctl(fildes, FIONBIO, &noblock)){
                return -1;
            }
            return -1; 
        }   
        else if(ret==0){
            noblock = 0;
            if(ioctl(fildes, FIONBIO, &noblock)){
                return -1;
            }
            return 0;
        }   
    }   

    noblock = 0;
    if(ioctl(fildes, FIONBIO, &noblock)){
        return -1;
    }
    return done;
}

extern ssize_t write_check(int fildes, const void* buf, size_t nbyte)
{
    size_t done = 0;
    ssize_t ret = 0;
    struct timeval timeout = {0, 0};
    fd_set read_set;
    fd_set error_set;
    int len = 0;

    if(nbyte==0){
        return 0;
    }   

    do{
        FD_ZERO(&read_set);
        FD_ZERO(&error_set);
        FD_SET(fildes, &read_set);
        FD_SET(fildes, &error_set);
        ret = select(fildes+1, &read_set, 0, &error_set, &timeout);
    }while(ret<0 && errno==EINTR);
    if(ret<0){
        return -1;
    }
    else if(ret>0){
        if(FD_ISSET(fildes, &error_set)){
            return -1;
        }
        if(ioctl(fildes, FIONREAD, &len)){
            return -1;
        }
        if(len==0){
            return -1;
        }
    }

    while(done<nbyte){
        ret = write(fildes, (char*)buf+done, nbyte-done);
        if(ret>0){
            done += ret;
        }   
        else if(ret<0 && errno!=EAGAIN && errno!=EINTR){
            return -1; 
        }   
    }   

    return done;
}

extern int connection_check(int fildes)
{
    struct timeval timeout = {0, 0};
    fd_set read_set;
    fd_set error_set;
    int len = 0;
    int ret = 0;

    do{
        FD_ZERO(&read_set);
        FD_ZERO(&error_set);
        FD_SET(fildes, &read_set);
        FD_SET(fildes, &error_set);
        ret = select(fildes+1, &read_set, 0, &error_set, &timeout);
    }while(ret<0 && errno==EINTR);
    if(ret<0){
        return -1;
    }
    else if(ret>0){
        if(FD_ISSET(fildes, &error_set)){
            return -1;
        }
        if(ioctl(fildes, FIONREAD, &len)){
            return -1;
        }
        if(len==0){
            return -1;
        }
    }

    return 0;
}
