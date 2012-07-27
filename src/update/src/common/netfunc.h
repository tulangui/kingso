#include <sys/types.h>
#include <sys/socket.h>

extern int listening(const unsigned short port);

extern int accept_wait(int socket, struct sockaddr* address, socklen_t* address_len, int* running);

extern int connect_wait(int socket, const struct sockaddr* address, socklen_t address_len, int* running);

extern int connect_times(int socket, const struct sockaddr* address, socklen_t address_len, int times);

extern ssize_t read_wait(int fildes, void* buf, size_t nbyte, int* running);

extern ssize_t write_check(int fildes, const void* buf, size_t nbyte);

extern int connection_check(int fildes);

