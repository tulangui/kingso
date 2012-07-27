#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "util/common.h"
#include "util/strfunc.h"

static void str_case_test()
{
#define LOWERCASE_STR  "this is a test!"
#define UPPERCASE_STR  "THIS IS A TEST!"

  char buff[256];

  strcpy(buff, "");
  assert(strcmp(str_lowercase(buff), "") == 0);

  strcpy(buff, LOWERCASE_STR);
  assert(strcmp(str_lowercase(buff), LOWERCASE_STR) == 0);

  strcpy(buff, "This is a test!");
  assert(strcmp(str_lowercase(buff), LOWERCASE_STR) == 0);

  strcpy(buff, "This Is A Test!");
  assert(strcmp(str_lowercase(buff), LOWERCASE_STR) == 0);

  strcpy(buff, UPPERCASE_STR);
  assert(strcmp(str_lowercase(buff), LOWERCASE_STR) == 0);

  strcpy(buff, UPPERCASE_STR);
  assert(strcmp(str_uppercase(buff), UPPERCASE_STR) == 0);

  strcpy(buff, "This is a test!");
  assert(strcmp(str_uppercase(buff), UPPERCASE_STR) == 0);

  strcpy(buff, "This Is A Test!");
  assert(strcmp(str_uppercase(buff), UPPERCASE_STR) == 0);

  strcpy(buff, LOWERCASE_STR);
  assert(strcmp(str_uppercase(buff), UPPERCASE_STR) == 0);

  printf("lowercase & uppercase test OK\n");
}

static void hex_bin_convert_test()
{
#define STR_BIN  "\x01\x02\x09\xA0\x70\r\ntest\r\n"
#define STR_HEX  "010209a0700d0a746573740d0a"

  char bin[256];
  char hex[512];
  int src_bin_len;
  int decode_bin_len;

  src_bin_len = sprintf(bin, "%s", STR_BIN);

  str_bin2hex(STR_BIN, src_bin_len, hex);
  assert(strcmp(hex, STR_HEX) == 0);

  str_hex2bin(hex, bin, &decode_bin_len);
  assert(decode_bin_len == src_bin_len);

  assert(memcmp(bin, STR_BIN, decode_bin_len) == 0);

  printf("bin hex convert test OK\n");
}

static void str_ltrim_test()
{
  char src[256];
  char dest[256];

  strcpy(src, "");
  strcpy(dest, "");
  str_ltrim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, " \r\n\t ");
  strcpy(dest, "");
  str_ltrim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "test");
  strcpy(dest, "test");
  str_ltrim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, " \r\n\t  test");
  strcpy(dest, "test");
  str_ltrim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "\r\n\ttest\r\n");
  strcpy(dest, "test\r\n");
  str_ltrim(src);
  assert(strcmp(src, dest) == 0);

  printf("str_ltrim test OK\n");
}

static void str_rtrim_test()
{
  char src[256];
  char dest[256];

  strcpy(src, "");
  strcpy(dest, "");
  str_rtrim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, " \r\n\t ");
  strcpy(dest, "");
  str_rtrim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "test");
  strcpy(dest, "test");
  str_rtrim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "test \r\n\t  ");
  strcpy(dest, "test");
  str_rtrim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "\r\n\ttest\r\n  ");
  strcpy(dest, "\r\n\ttest");
  str_rtrim(src);
  assert(strcmp(src, dest) == 0);

  printf("str_rtrim test OK\n");
}

static void str_trim_test()
{
  char src[256];
  char dest[256];

  strcpy(src, "");
  strcpy(dest, "");
  str_trim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, " \r\n\t ");
  strcpy(dest, "");
  str_trim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "test");
  strcpy(dest, "test");
  str_trim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "\r\n\ttest");
  strcpy(dest, "test");
  str_trim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "test \r\n\t  ");
  strcpy(dest, "test");
  str_trim(src);
  assert(strcmp(src, dest) == 0);

  strcpy(src, "\r\n\ttest\r\n  ");
  strcpy(dest, "test");
  str_trim(src);
  assert(strcmp(src, dest) == 0);

  printf("str_trim test OK\n");
}

static void str_split_test()
{
  char src[256];
  char **cols;
  int col_count;
  char seperator;

  strcpy(src, "");
  seperator = ' ';
  cols = str_split(src, seperator, 0, &col_count);
  assert(cols != NULL);
  assert(col_count == 1);
  assert(strcmp(cols[0], "") == 0);
  str_free_split(cols);

  strcpy(src, "this is a test");
  seperator = ',';
  cols = str_split(src, seperator, 0, &col_count);
  assert(cols != NULL);
  assert(col_count == 1);
  assert(strcmp(cols[0], "this is a test") == 0);
  str_free_split(cols);

  strcpy(src, "=");
  seperator = '=';
  cols = str_split(src, seperator, 0, &col_count);
  assert(cols != NULL);
  assert(col_count == 2);
  assert(strcmp(cols[0], "") == 0);
  assert(strcmp(cols[1], "") == 0);
  str_free_split(cols);

  strcpy(src, ",,,");
  seperator = ',';
  cols = str_split(src, seperator, 0, &col_count);
  assert(cols != NULL);
  assert(col_count == 4);
  assert(strcmp(cols[0], "") == 0);
  assert(strcmp(cols[1], "") == 0);
  assert(strcmp(cols[2], "") == 0);
  assert(strcmp(cols[3], "") == 0);
  str_free_split(cols);

  strcpy(src, "this is a test");
  seperator = ' ';
  cols = str_split(src, seperator, 0, &col_count);
  assert(cols != NULL);
  assert(col_count == 4);
  assert(strcmp(cols[0], "this") == 0);
  assert(strcmp(cols[1], "is") == 0);
  assert(strcmp(cols[2], "a") == 0);
  assert(strcmp(cols[3], "test") == 0);
  str_free_split(cols);

  printf("str_split test OK\n");
}

static void str_split_ex_test()
{
#define MAX_COLS  4

  char src[256];
  char *cols[MAX_COLS];
  int col_count;
  char seperator;

  strcpy(src, "");
  seperator = ' ';
  col_count = str_split_ex(src, seperator, cols, MAX_COLS);
  assert(col_count == 1);
  assert(strcmp(cols[0], "") == 0);

  strcpy(src, "this is a test");
  seperator = ',';
  col_count = str_split_ex(src, seperator, cols, MAX_COLS);
  assert(col_count == 1);
  assert(strcmp(cols[0], "this is a test") == 0);

  strcpy(src, "=");
  seperator = '=';
  col_count = str_split_ex(src, seperator, cols, MAX_COLS);
  assert(col_count == 2);
  assert(strcmp(cols[0], "") == 0);
  assert(strcmp(cols[1], "") == 0);

  strcpy(src, ",,,");
  seperator = ',';
  col_count = str_split_ex(src, seperator, cols, MAX_COLS);
  assert(col_count == 4);
  assert(strcmp(cols[0], "") == 0);
  assert(strcmp(cols[1], "") == 0);
  assert(strcmp(cols[2], "") == 0);
  assert(strcmp(cols[3], "") == 0);

  strcpy(src, "this is a test");
  seperator = ' ';
  col_count = str_split_ex(src, seperator, cols, MAX_COLS);
  assert(col_count == 4);
  assert(strcmp(cols[0], "this") == 0);
  assert(strcmp(cols[1], "is") == 0);
  assert(strcmp(cols[2], "a") == 0);
  assert(strcmp(cols[3], "test") == 0);

  strcpy(src, "this is a test ");
  seperator = ' ';
  col_count = str_split_ex(src, seperator, cols, MAX_COLS);
  assert(col_count == 4);
  assert(strcmp(cols[0], "this") == 0);
  assert(strcmp(cols[1], "is") == 0);
  assert(strcmp(cols[2], "a") == 0);
  assert(strcmp(cols[3], "test ") == 0);
  printf("str_split_ex test OK\n");
}

void split_multi_delim_test(){
    char a[]="1";
    char *items[10];
    int ret;
    ret = split_multi_delims(a,items,1,"2");
    printf("ret=%d\n",ret);
    return;
}

int main(int argc,char *argv[])
{
  str_case_test();
  hex_bin_convert_test();
  str_ltrim_test();
  str_rtrim_test();

  str_trim_test();

  str_split_test();
  str_split_ex_test();
    
  split_multi_delim_test();

  printf("test OK\n");
  return 0;
}

