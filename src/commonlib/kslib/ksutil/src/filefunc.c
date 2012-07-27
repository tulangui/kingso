#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/limits.h>

#include "util/filefunc.h"

bool file_exist(const char *filename)
{
    return access(filename, 0) == 0;
}

int file_get_content(const char *filename, char **buff, int64_t *file_size)
{
    int fd;
    int result;

    *buff = NULL;
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        *file_size = 0;
        return errno != 0 ? errno : ENOENT;
    }

    do {
        if ((*file_size=lseek(fd, 0, SEEK_END)) < 0) {
            result = errno != 0 ? errno : EIO;
            break;
        }

        *buff = (char *)malloc(*file_size + 1);
        if (*buff == NULL) {
            result = errno != 0 ? errno : ENOMEM;
            break;
        }

        if (lseek(fd, 0, SEEK_SET) < 0) {
            result = errno != 0 ? errno : EIO;
            break;
        }
        if (read(fd, *buff, *file_size) != *file_size) {
            result = errno != 0 ? errno : EIO;
            break;
        }

        (*buff)[*file_size] = '\0';
        result = 0;
    } while (false);

    close(fd);

    if (result != 0) {
        if (*buff != NULL) {
            free(*buff);
            *buff = NULL;
        }

        *file_size = 0;
    }

    return result;
}

int file_write_buffer(const char *filename, const char *buff, 
        const int file_size)
{
    int fd;
    int result;

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return errno != 0 ? errno : EIO;
    }

    if (write(fd, buff, file_size) != file_size) {
        result = errno != 0 ? errno : EIO;
        close(fd);
        return result;
    }

    close(fd);
    return 0;
}

static int split_path(char * path, char ** pathes) {
	char * ptr;
	int i;
	i = 0;
	while( *path != 0 ) {
		ptr = strchr(path, '/');
		if( ptr == NULL ) {
			pathes[i++] = path;
			break;
		}
		*ptr = 0;
		if( path != ptr ) {
			pathes[i++] = path;
		}
		path = ptr + 1;
	}
	return i;
}

char * get_realpath(const char * path, char * rpath) {
	char pwd[PATH_MAX + 1];
	char vpath[PATH_MAX + 1];
	int len;
	int level, level2;
	char * pathes[PATH_MAX / 2 + 1];
	char * pathes2[PATH_MAX / 2 + 1];
	int i;
	if( path == NULL || rpath == NULL ) {
		errno = EINVAL;
		return NULL;
	}
	if( path[0] != '/' ) {
		if( getcwd(pwd, PATH_MAX + 1) == NULL ) {
			return NULL;
		}
		level = split_path(pwd, pathes);
	} else {
		level = 0;
	}
	strcpy(vpath, path);
	level2 = split_path(vpath, pathes2);
	for( i = 0; i < level2; i++ ) {
		if( pathes2[i][0] == 0 || strcmp(pathes2[i], ".") == 0 ) {
		} else if( strcmp(pathes2[i], "..") == 0 ) {
			level--;
		} else {
			pathes[level++] = pathes2[i];
		}
	}
	strcpy(rpath, "/");
	for( i = 0; i < level - 1; i++ ) {
		strcat(rpath, pathes[i]);
		strcat(rpath, "/");
	}
	strcat(rpath, pathes[i]);
	return rpath;
}

int make_dir(const char *patchname, mode_t mode){
    struct stat stbuf;
    if (stat(patchname, &stbuf) == 0) {
        if ((S_IFDIR & stbuf.st_mode) != 0) {
            return 0;
        }
    }

    return mkdir(patchname, mode);
}
