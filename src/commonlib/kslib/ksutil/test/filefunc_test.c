#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "util/common.h"
#include "util/filefunc.h"

int main(int argc,char *argv[])
{
  char filename[256];
  char buff[16 * 1024];
  char *content;
  int64_t file_size;

  strcpy(filename, "/tmp/.filefunc.test");
  if (file_exist(filename)) {
    if (unlink(filename) != 0) {
      fprintf(stderr, "file: "__FILE__", line: %d, " \
          "unlinke file: %s fail, " \
          "errno: %d, error info: %s\n", __LINE__, \
          filename, errno, strerror(errno));
      return errno;
    }
  }

  assert(!file_exist(filename));
  memset(buff, 'a', sizeof(buff));

  assert(file_write_buffer(filename, buff, sizeof(buff)) == 0);
  assert(file_exist(filename));

  assert(file_get_content(filename, &content, &file_size) == 0);

  assert(file_size == sizeof(buff));
  assert(memcmp(buff, content, file_size) == 0);

  free(content);
  assert(unlink(filename) == 0);
  assert(!file_exist(filename));
  assert(file_get_content(filename, &content, &file_size) == ENOENT);

  printf("test OK\n");

  return 0;
}

