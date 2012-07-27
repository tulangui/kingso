#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "util/common.h"
#include "util/filefunc.h"
#include "util/iniparser.h"

#define CONF_FILENAME "ini_parser_test.conf"

static void test_ini_items(ini_context_t *pContext)
{
#define MAX_VALUES  8
  char *values[MAX_VALUES];
  int count;

  assert(ini_get_str_value(NULL, "item_none", pContext) == NULL);
  assert(ini_get_str_value(NULL, "bind_addr", pContext) != NULL);
  assert(strcmp(ini_get_str_value(NULL, "bind_addr", pContext), "127.0.0.1") == 0);

  assert(ini_get_str_value(NULL, "param_str_empty", pContext) != NULL);
  assert(strcmp(ini_get_str_value(NULL, "param_str_empty", pContext), "") == 0);

  assert(ini_get_int_value("section1", "param_int_1", pContext, 0) == -1);
  assert(ini_get_int_value("section2", "param_int21", pContext, 0) == 21);
  assert(ini_get_int64_value("section2", "param_int64", pContext, 0) == 12345678901234LL);
  assert(ini_get_double_value("section1", "param_double1", pContext, 0.00) == 1234.05);
  assert(ini_get_double_value("section1", "param_double2", pContext, 0.00) == 1234.0);

  assert(!ini_get_bool_value(NULL, "global_bool0", pContext, false));
  assert(!ini_get_bool_value(NULL, "global_bool_no_exist", pContext, false));
  assert(ini_get_bool_value(NULL, "global_bool_true", pContext, false));
  assert(!ini_get_bool_value(NULL, "global_bool_false", pContext, false));
  assert(ini_get_bool_value(NULL, "global_bool1", pContext, false));

  assert(ini_get_str_values("section2", "param_str", pContext, values, MAX_VALUES) == 4);
  assert(ini_get_str_values("section2", "not_exist_item", pContext, values, MAX_VALUES) == 0);
  assert(ini_get_str_values_ex("section2", "param_str", pContext, &count) != NULL);
  assert(count == 4);

  assert(ini_get_int_value("", "param_child_int1", pContext, 0) == 123456);
  assert(ini_get_str_values("", "param_child_str1", pContext, values, MAX_VALUES) == 2);
}

static void test_ini_items1(ini_context_t *pContext)
{
#define MAX_VALUES  8
  const ini_section_t *global_section;
  const ini_section_t *section1;
  const ini_section_t *section2;
  char *values[MAX_VALUES];
  int count;

  global_section = ini_get_section(NULL, pContext);
  assert(global_section != NULL);

  assert(ini_get_str_value1(global_section, "item_none") == NULL);
  assert(ini_get_str_value1(global_section, "bind_addr") != NULL);
  assert(strcmp(ini_get_str_value1(global_section, "bind_addr"), "127.0.0.1") == 0);

  assert(ini_get_str_value1(global_section, "param_str_empty") != NULL);
  assert(strcmp(ini_get_str_value1(global_section, "param_str_empty"), "") == 0);
  assert(!ini_get_bool_value1(global_section, "global_bool0", false));
  assert(!ini_get_bool_value1(global_section, "global_bool_no_exist", false));
  assert(ini_get_bool_value1(global_section, "global_bool_true", false));
  assert(!ini_get_bool_value1(global_section, "global_bool_false", false));
  assert(ini_get_bool_value1(global_section, "global_bool1", false));

  global_section = ini_get_section("", pContext);
  assert(global_section != NULL);

  assert(ini_get_int_value1(global_section, "param_child_int1", 0) == 123456);
  assert(ini_get_str_values1(global_section, "param_child_str1", values, MAX_VALUES) == 2);

  section1 = ini_get_section("section1", pContext);
  assert(section1 != NULL);

  assert(ini_get_int_value1(section1, "param_int_1", 0) == -1);
  assert(ini_get_double_value1(section1, "param_double1", 0.00) == 1234.05);
  assert(ini_get_double_value1(section1, "param_double2", 0.00) == 1234.0);

  section2 = ini_get_section("section2", pContext);
  assert(section2 != NULL);

  assert(ini_get_int_value1(section2, "param_int21", 0) == 21);
  assert(ini_get_int64_value1(section2, "param_int64", 0) == 12345678901234LL);
  assert(ini_get_str_values1(section2, "param_str", values, MAX_VALUES) == 4);
  assert(ini_get_str_values1(section2, "not_exist_item", values, MAX_VALUES) == 0);
  assert(ini_get_str_values_ex1(section2, "param_str", &count) != NULL);
  assert(count == 4);
}

static int test_load_from_buffer()
{
  ini_context_t context;
  char *buffer;
  int64_t file_size;
  int result;

  if ((result=file_get_content(CONF_FILENAME, &buffer, &file_size)) != 0) {
    perror("file_get_content");
    return result;
  }

  assert(ini_load_from_buffer(buffer, &context) == 0);

  //ini_print_items(&context);

  test_ini_items(&context);
  test_ini_items1(&context);

  ini_destroy(&context);
  free(buffer);

  printf("load from buffer test OK\n");

  return 0;
}

static int test_load_from_file()
{
  ini_context_t context;

  assert(ini_load_from_file(CONF_FILENAME, &context) == 0);

  test_ini_items(&context);
  test_ini_items1(&context);

  ini_destroy(&context);

  printf("load from file test OK\n");

  return 0;
}

int main(int argc,char *argv[])
{
  test_load_from_buffer();
  test_load_from_file();

  printf("test OK\n");

  return 0;
}

