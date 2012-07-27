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
 * $Id: sync.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

// write pipe
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char** argv)
{
    if(argc != 3)
        return -1;
    int fd;
    umask(0);
    if((mkfifo(argv[2], (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) && (errno != EEXIST)) {
        printf("mkfifo error, name=%s\n", argv[2]);
        return -1;
    }
    fd = open(argv[2], O_WRONLY | O_NONBLOCK);
    if(fd < 0) {
        printf("open %s error, info=%s\n", argv[2], strerror(errno));
        return -1;
    }
    write(fd, argv[1], 1);
    close(fd);
    printf("write ok\n");

    return 0;
}
