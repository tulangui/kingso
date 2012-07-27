#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "util/strfunc.h"
#include "util/hashfunc.h"
#include "util/filefunc.h"
#include "util/iniparser.h"

#define _LINE_BUFFER_SIZE	512
#define _ALLOC_ITEMS_ONCE	32

static int ini_do_load_from_file(const char *filename, 
        ini_context_t *context);
static int ini_do_load_from_buffer(char *content, 
        ini_context_t *context);

static int ini_compare_by_item_name(const void *p1, const void *p2)
{
    return strcmp(((ini_item_t *)p1)->name, ((ini_item_t *)p2)->name);
}

static int32_t ini_compare_key(const void *key1, const void *key2)
{
    return strcmp((const char *)key1, (const char *)key2);
}

static int ini_init_context(ini_context_t *context)
{
    const int buckets = 511;
    int result;

    memset(context, 0, sizeof(ini_context_t));
    context->current_section = &context->global;

    context->sections = hashmap_create(buckets, HM_FLAG_POOL, 
            time33_hash_func, ini_compare_key);
    if (context->sections == NULL) {
        result = errno != 0 ? errno : ENOMEM;
        fprintf(stderr, "file: "__FILE__", line: %d, " 
                "hashmap_create fail, errno: %d, error info: %s", 
                __LINE__, result, strerror(result));
        return result;
    }

    return 0;
}

static int ini_sort_data_func(void *key, void *value, void *user_data)
{
    ini_section_t *section;

    section = (ini_section_t *)value;
    if (section->count > 1) {
        qsort(section->items, section->count, 
                sizeof(ini_item_t), ini_compare_by_item_name);
    }

    return 0;
}

static void ini_sort_items(ini_context_t *context)
{
    if (context->global.count > 1) {
        qsort(context->global.items, context->global.count, 
                sizeof(ini_item_t), ini_compare_by_item_name);
    }

    hashmap_walk(context->sections, ini_sort_data_func, NULL);
}

int ini_load_from_file(const char *filename, ini_context_t *context)
{
    int result;
    char *last_ptr;
    char full_filename[PATH_MAX];
    char old_cwd[PATH_MAX];

    memset(old_cwd, 0, sizeof(old_cwd));
    if (strncasecmp(filename, "http://", 7) != 0) {
        last_ptr = strrchr(filename, '/');
        if (last_ptr != NULL) {
            char path[256];
            int len;

            if (getcwd(old_cwd, sizeof(old_cwd)) == NULL) {
                fprintf(stderr, "file: "__FILE__", line: %d, " 
                        "getcwd fail, errno: %d, " 
                        "error info: %s", 
                        __LINE__, errno, strerror(errno));
                *old_cwd = '\0';
            }

            len = last_ptr - filename;
            if (len > 0) {
                if (len >= sizeof(path)) {
                    len = sizeof(path) - 1;
                }
                memcpy(path, filename, len);
                *(path + len) = '\0';
                if (chdir(path) != 0) {
                    fprintf(stderr, "file: "__FILE__", line: %d, "
                            "chdir to the path of conf " 
                            "file: %s fail, errno: %d, " 
                            "error info: %s", 
                            __LINE__, filename, 
                            errno, strerror(errno));
                    return errno != 0 ? errno : ENOENT;
                }
            }
        }
    }

    if (*filename != '/' && *old_cwd != '\0') {
        snprintf(full_filename, sizeof(full_filename), "%s/%s", 
                old_cwd, filename);
    }
    else {
        snprintf(full_filename, sizeof(full_filename),"%s",filename);
    }

    if ((result=ini_init_context(context)) != 0) {
        return result;
    }

    result = ini_do_load_from_file(full_filename, context);
    if (result == 0) {
        ini_sort_items(context);
    }
    else {
        ini_destroy(context);
    }

    if (*old_cwd != '\0' && chdir(old_cwd) != 0) {
        fprintf(stderr, "file: "__FILE__", line: %d, " 
                "chdir to old path: %s fail, " 
                "errno: %d, error info: %s", 
                __LINE__, old_cwd, errno, strerror(errno));
        return errno != 0 ? errno : ENOENT;
    }

    return result;
}

static int ini_do_load_from_file(const char *filename, 
        ini_context_t *context)
{
    int64_t file_size;
    char *content;
    int result;

    /*
     * TODO: support URL like http://xxx
     */
    /*
       int http_status;
       int content_len;
       char error_info[512];
       if (strncasecmp(filename, "http://", 7) == 0) {
       if ((result=get_url_content(filename, 10, 60, &http_status, 
       &content, &content_len, error_info)) != 0)
       {
       fprintf(stderr, "file: "__FILE__", line: %d, " 
       "get_url_content fail, " 
       "url: %s, error info: %s", 
       __LINE__, filename, error_info);
       return result;
       }

       if (http_status != 200) {
       free(content);
       fprintf(stderr, "file: "__FILE__", line: %d, " 
       "HTTP status code: %d != 200, url: %s", 
       __LINE__, http_status, filename);
       return EINVAL;
       }
       }
       else
       */

    {
        if ((result=file_get_content(filename, &content, &file_size)) != 0) {
            return result;
        }
    }

    result = ini_do_load_from_buffer(content, context);
    free(content);

    return result;
}

int ini_load_from_buffer(char *content, ini_context_t *context)
{
    int result;

    if ((result=ini_init_context(context)) != 0) {
        return result;
    }

    result = ini_do_load_from_buffer(content, context);
    if (result == 0) {
        ini_sort_items(context);
    }
    else {
        ini_destroy(context);
    }

    return result;
}

static int ini_do_load_from_buffer(char *content, ini_context_t *context)
{
    ini_section_t *section;
    ini_item_t *item;
    char *line;
    char *last_end_ptr;
    char *equal_ptr;
    char *include_filename;
    int line_len;
    int name_len;
    int value_len;
    int result;

    result = 0;
    last_end_ptr = content - 1;
    section = context->current_section;
    if (section->count > 0) {
        item = section->items + section->count;
    }
    else {
        item = section->items;
    }

    while (last_end_ptr != NULL) {
        line = last_end_ptr + 1;
        last_end_ptr = strchr(line, '\n');
        if (last_end_ptr != NULL) {
            *last_end_ptr = '\0';
        }

        if (*line == '#' && 
                strncasecmp(line+1, "include", 7) == 0 && 
                (*(line+8) == ' ' || *(line+8) == '\t'))
        {
            include_filename = strdup(line + 9);
            if (include_filename == NULL) {
                fprintf(stderr, "file: "__FILE__", line: %d, " 
                        "strdup %d bytes fail", __LINE__, 
                        (int)strlen(line + 9) + 1);
                result = errno != 0 ? errno : ENOMEM;
                break;
            }

            str_trim(include_filename);
            if (strncasecmp(include_filename, "http://", 7) != 0 
                    && !file_exist(include_filename))
            {
                fprintf(stderr, "file: "__FILE__", line: %d, " 
                        "include file \"%s\" not exists, " 
                        "line: \"%s\"", __LINE__, 
                        include_filename, line);
                free(include_filename);
                result = ENOENT;
                break;
            }

            result = ini_do_load_from_file(include_filename, 
                    context);
            if (result != 0) {
                free(include_filename);
                break;
            }

            section = context->current_section;
            if (section->count > 0) {
                item = section->items + section->count;  //must re-asign
            }
            else {
                item = section->items;
            }

            free(include_filename);
            continue;
        }

        str_trim(line);
        if (*line == '#' || *line == '\0') {
            continue;
        }

        line_len = strlen(line);
        if (*line == '[' && *(line + (line_len - 1)) == ']') { //section
            char *section_name;
            int section_len;

            *(line + (line_len - 1)) = '\0';
            section_name = line + 1; //skip [

            str_trim(section_name);
            if (*section_name == '\0') { //global section
                context->current_section = &context->global;
                section = context->current_section;
                if (section->count > 0) {
                    item = section->items + section->count;
                }
                else {
                    item = section->items;
                }
                continue;
            }

            section_len = strlen(section_name);
            if (section_len > INI_SECTION_NAME_LEN) {
                fprintf(stderr, "file: "__FILE__", line: %d, "
                        "section: %s, name is too long, exceeds %d", 
                        __LINE__, section_name, INI_SECTION_NAME_LEN);
                result = ENAMETOOLONG;
                break;
            }

            section = (ini_section_t *)hashmap_find(context->sections, 
                    section_name);
            if (section == NULL) {
                section = (ini_section_t *)malloc(sizeof(ini_section_t));
                if (section == NULL) {
                    result = errno != 0 ? errno : ENOMEM;
                    fprintf(stderr, "file: "__FILE__", line: %d, "
                            "malloc %d bytes fail, " 
                            "errno: %d, error info: %s", 
                            __LINE__, 
                            (int)sizeof(ini_section_t), 
                            result, strerror(result));

                    break;
                }

                memset(section, 0, sizeof(ini_section_t));
                memcpy(section->section_name, section_name, section_len);
                result = hashmap_insert(context->sections, 
                        section->section_name, section);
                if (result < 0) {
                    result = errno != 0 ? errno : ENOMEM;
                    fprintf(stderr, "file: "__FILE__", line: %d, "
                            "insert into hash table fail, "
                            "errno: %d, error info: %s", 
                            __LINE__, result, 
                            strerror(result));
                    break;
                }
                else {
                    result = 0;
                }
            }

            context->current_section = section;
            if (section->count > 0) {
                item = section->items + section->count;
            }
            else {
                item = section->items;
            }
            continue;
        }

        equal_ptr = strchr(line, '=');
        if (equal_ptr == NULL) {
            continue;
        }

        name_len = equal_ptr - line;
        value_len = strlen(line) - (name_len + 1);
        if (name_len > INI_ITEM_NAME_LEN) {
            name_len = INI_ITEM_NAME_LEN;
        }

        if (value_len > INI_ITEM_VALUE_LEN) {
            value_len = INI_ITEM_VALUE_LEN;
        }

        if (section->count >= section->alloc_count) {
            section->alloc_count += _ALLOC_ITEMS_ONCE;
            section->items=(ini_item_t *)realloc(section->items, 
                    sizeof(ini_item_t) * section->alloc_count);
            if (section->items == NULL) {
                fprintf(stderr, "file: "__FILE__", line: %d, " 
                        "realloc %d bytes fail", __LINE__, 
                        (int)sizeof(ini_item_t) * 
                        section->alloc_count);
                result = errno != 0 ? errno : ENOMEM;
                break;
            }

            item = section->items + section->count;
            memset(item, 0, sizeof(ini_item_t) * 
                    (section->alloc_count - section->count));
        }

        memcpy(item->name, line, name_len);
        memcpy(item->value, equal_ptr + 1, value_len);

        str_trim(item->name);
        str_trim(item->value);

        section->count++;
        item++;
    }

    return result;
}

static void ini_free_hash_data_func(void *key, void *value, void *user_data)
{
    ini_section_t *section;

    section = (ini_section_t *)value;
    if (section == NULL) {
        return;
    }

    if (section->items != NULL) {
        free(section->items);
        memset(section, 0, sizeof(ini_section_t));
    }

    free(section);
    return;
}

void ini_destroy(ini_context_t *context)
{
    if (context == NULL) {
        return;
    }

    if (context->global.items != NULL) {
        free(context->global.items);
        memset(&context->global, 0, sizeof(ini_section_t));
    }

    hashmap_destroy(context->sections, ini_free_hash_data_func, NULL);
}

const char *ini_get_str_value(const char *section_name, 
        const char *item_name, const ini_context_t *context)
{
    const ini_section_t *section;

    section = ini_get_section(section_name, context);
    if (section == NULL) {
        return NULL;
    }

    return ini_get_str_value1(section, item_name);
}

int ini_get_int_value(const char *section_name, const char *item_name, 
        const ini_context_t *context, const int default_value)
{
    const ini_section_t *section;

    section = ini_get_section(section_name, context);
    if (section == NULL) {
        return default_value;
    }

    return ini_get_int_value1(section, item_name, default_value);
}

int64_t ini_get_int64_value(const char *section_name, 
        const char *item_name, const ini_context_t *context, 
        const int64_t default_value)
{
    const ini_section_t *section;

    section = ini_get_section(section_name, context);
    if (section == NULL) {
        return default_value;
    }

    return ini_get_int64_value1(section, item_name, default_value);
}

double ini_get_double_value(const char *section_name, 
        const char *item_name, const ini_context_t *context, 
        const double default_value)
{
    const ini_section_t *section;

    section = ini_get_section(section_name, context);
    if (section == NULL) {
        return default_value;
    }

    return ini_get_double_value1(section, item_name, default_value);
}

bool ini_get_bool_value(const char *section_name, const char *item_name, 
        const ini_context_t *context, const bool default_value)
{
    const ini_section_t *section;

    section = ini_get_section(section_name, context);
    if (section == NULL) {
        return default_value;
    }

    return ini_get_bool_value1(section, item_name, default_value);
}

int ini_get_str_values(const char *section_name, const char *item_name, 
        const ini_context_t *context, char **values, const int max_values)
{
    const ini_section_t *section;

    section = ini_get_section(section_name, context);
    if (section == NULL) {
        return 0;
    }

    return ini_get_str_values1(section, item_name, values, max_values);
}

const ini_item_t *ini_get_str_values_ex(const char *section_name, 
        const char *item_name, const ini_context_t *context, 
        int *found_count)
{
    const ini_section_t *section;

    section = ini_get_section(section_name, context);
    if (section == NULL) {
        *found_count = 0;
        return NULL;
    }

    return ini_get_str_values_ex1(section, item_name, found_count);
}

const ini_section_t *ini_get_section(const char *section_name, 
        const ini_context_t *context)
{
    if (section_name == NULL || *section_name == '\0') {
        return &context->global;
    }
    else {
        return (ini_section_t *)hashmap_find(context->sections, 
                (void *)section_name);
    }
}

const char *ini_get_str_value1(const ini_section_t *section, 
        const char *item_name)
{
    ini_item_t target_item;
    ini_item_t *found;

    if (section == NULL || section->count <= 0) {
        return NULL;
    }

    snprintf(target_item.name, sizeof(target_item.name), "%s", item_name);
    found = (ini_item_t *)bsearch(&target_item, section->items, 
            section->count, sizeof(ini_item_t), ini_compare_by_item_name);
    if (found == NULL) {
        return NULL;
    }

    return found->value;
}

int ini_get_int_value1(const ini_section_t *section, 
        const char *item_name, const int default_value)
{
    const char *str;

    str = ini_get_str_value1(section, item_name);
    if (str == NULL) {
        return default_value;
    }
    else {
        return atoi(str);
    }
}

int64_t ini_get_int64_value1(const ini_section_t *section, 
        const char *item_name, const int64_t default_value)
{
    const char *str;

    str = ini_get_str_value1(section, item_name);
    if (str == NULL) {
        return default_value;
    }
    else {
        return strtoll(str, NULL, 10);
    }
}

bool ini_get_bool_value1(const ini_section_t *section, 
        const char *item_name, const bool default_value)
{
    const char *str;

    str = ini_get_str_value1(section, item_name);
    if (str == NULL) {
        return default_value;
    }
    else {
        return (strcasecmp(str, "true") == 0 ||
            strcasecmp(str, "yes") == 0 ||
            strcasecmp(str, "on") == 0 ||
            strcmp(str, "1") == 0);
    }
}

double ini_get_double_value1(const ini_section_t *section, 
        const char *item_name, const double default_value)
{
    const char *str;

    str = ini_get_str_value1(section, item_name);
    if (str == NULL) {
        return default_value;
    }
    else {
        return strtod(str, NULL);
    }
}

int ini_get_str_values1(const ini_section_t *section, const char *item_name, 
        char **values, const int max_values)
{
    ini_item_t target_item;
    ini_item_t *found;
    ini_item_t *item;
    ini_item_t *item_end;
    char **pp;

    if (section == NULL || max_values <= 0 || section->count <= 0) {
        return 0;
    }

    snprintf(target_item.name, sizeof(target_item.name), "%s", item_name);
    found = (ini_item_t *)bsearch(&target_item, section->items, 
            section->count, sizeof(ini_item_t), ini_compare_by_item_name);
    if (found == NULL) {
        return 0;
    }

    pp = values;
    *pp++ = found->value;
    for (item=found-1; item>=section->items; item--) {
        if (strcmp(item->name, item_name) != 0) {
            break;
        }

        if (pp - values < max_values) {
            *pp++ = item->value;
        }
    }

    item_end = section->items + section->count;
    for (item=found+1; item<item_end; item++) {
        if (strcmp(item->name, item_name) != 0) {
            break;
        }

        if (pp - values < max_values) {
            *pp++ = item->value;
        }
    }

    return pp - values;
}

const ini_item_t *ini_get_str_values_ex1(const ini_section_t *section, 
        const char *item_name, int *found_count)
{
    ini_item_t *item;
    ini_item_t *item_end;
    ini_item_t *item_start;
    ini_item_t target_item;
    ini_item_t *found;

    if (section == NULL || section->count <= 0) {
        *found_count = 0;
        return NULL;
    }

    snprintf(target_item.name, sizeof(target_item.name), "%s", item_name);
    found = (ini_item_t *)bsearch(&target_item, section->items, 
            section->count, sizeof(ini_item_t), ini_compare_by_item_name);
    if (found == NULL) {
        *found_count = 0;
        return NULL;
    }

    *found_count = 1;
    for (item=found-1; item>=section->items; item--) {
        if (strcmp(item->name, item_name) != 0) {
            break;
        }

        (*found_count)++;
    }
    item_start = found - (*found_count) + 1;

    item_end = section->items + section->count;
    for (item=found+1; item<item_end; item++) {
        if (strcmp(item->name, item_name) != 0) {
            break;
        }

        (*found_count)++;
    }

    return item_start;
}

static int ini_print_hash_data(void* key, void *value, void *user_data)
{
    ini_section_t *section;
    ini_item_t *item;
    ini_item_t *item_end;
    const char *section_name;
    int i;

    section = (ini_section_t *)value;
    if (section == NULL) {
        return 0;
    }

    section_name = (const char *)key;
    printf("section: %s, item count: %d\n", section_name, section->count);
    if (section->count > 0) {
        i = 0;
        item_end = section->items + section->count;
        for (item=section->items; item<item_end; item++) {
            printf("%d. %s=%s\n", ++i, item->name, item->value);
        }
    }
    printf("\n");

    return 0;
}

void ini_print_items(const ini_context_t *context)
{
    ini_item_t *item;
    ini_item_t *item_end;
    int i;

    printf("global section, item count: %d\n", context->global.count);
    if (context->global.count > 0) {
        i = 0;
        item_end = context->global.items + context->global.count;
        for (item=context->global.items; item<item_end; item++) {
            printf("%d. %s=%s\n", ++i, item->name, item->value);
        }
    }
    printf("\n");

    hashmap_walk(context->sections, ini_print_hash_data, NULL);
}

