/** \file
 *******************************************************************
 * $Author: zhuangyuan $
 *
 * $LastChangedBy: zhuangyuan $
 *
 * $Revision: 44 $
 *
 * $LastChangedDate: 2011-03-18 17:06:54 +0800 (Fri, 18 Mar 2011) $
 *
 * $Id: iniparser.h 44 2011-03-18 09:06:54Z zhuangyuan $
 *
 * $Brief: ini file parser $
 *******************************************************************
 */

#ifndef INI_PARSER_H
#define INI_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "hashmap.h"

#define INI_SECTION_NAME_LEN 64
#define INI_ITEM_NAME_LEN    64
#define INI_ITEM_VALUE_LEN  256

typedef struct
{
    char name[INI_ITEM_NAME_LEN + 1];
    char value[INI_ITEM_VALUE_LEN + 1];
} ini_item_t;

typedef struct
{
    char section_name[INI_SECTION_NAME_LEN + 1];
    ini_item_t *items;   //key value pair array
    int count;  //item count
    int alloc_count; //alloc item count
} ini_section_t;

typedef struct
{
    ini_section_t global; //the global section
    hashmap_t sections;  //key is section name, and value is ini_section_t
    ini_section_t *current_section; //for load from ini file
} ini_context_t;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief load ini items from file
     * @param filename the filename
     * @param context the ini context, must call ini_destroy(context) when finish
     * @return error no, 0 for success, != 0 fail
     */
    int ini_load_from_file(const char *filename, ini_context_t *context);

    /**
     * @brief load ini items from string buffer
     * @param content the string buffer to parse, the content will be modified 
     *                by this function
     * @param context the ini context, must call ini_destroy(context) when finish
     * @return error no, 0 for success, != 0 fail
     */
    int ini_load_from_buffer(char *content, ini_context_t *context);

    /**
     * @brief destroy ini context
     * @param context the ini context
     */
    void ini_destroy(ini_context_t *context);

    /**
     * @brief get item string value
     * @param section_name the section name, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @return item value, return NULL when the item not exist
     */
    const char *ini_get_str_value(const char *section_name, 
            const char *item_name, const ini_context_t *context);

    /**
     * @brief get item int value (32 bits)
     * @param section_name the section name, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param default_value the default value
     * @return item value, return default_value when the item not exist
     */
    int ini_get_int_value(const char *section_name, const char *item_name,
            const ini_context_t *context, const int default_value);

    /**
     * @brief get item int64 value (64 bits)
     * @param section_name the section name, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param default_value the default value
     * @return int64 value, return default_value when the item not exist
     */
    int64_t ini_get_int64_value(const char *section_name, const char *item_name, 
            const ini_context_t *context, const int64_t default_value);

    /**
     * @brief get item boolean value
     * @param section_name the section name, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param default_value the default value
     * @return item boolean value, return default_value when the item not exist
     */
    bool ini_get_bool_value(const char *section_name, const char *item_name, 
            const ini_context_t *context, const bool default_value);

    /**
     * @brief get item double value
     * @param section_name the section name, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param default_value the default value
     * @return item value, return default_value when the item not exist
     */
    double ini_get_double_value(const char *section_name, 
            const char *item_name, const ini_context_t *context, 
            const double default_value);

    /**
     * @brief get item string values
     * @param section_name the section name, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param values   string array to store the values
     * @param max_values max string array elements
     * @return item value count, return 0 when the item not exist
     */
    int ini_get_str_values(const char *section_name, const char *item_name, 
            const ini_context_t *context, char **values, const int max_values);

    /**
     * @brief get item string value array
     * @param section_name the section name, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param found_count store the item value count
     * @return item value array, return NULL when the item not exist
     */
    const ini_item_t *ini_get_str_values_ex(const char *section_name, 
            const char *item_name, const ini_context_t *context, 
            int *found_count);

    /**
     * @brief get section pointer by name
     * @param section_name the section name, NULL or empty string for 
     *                      global section
     * @param context  the ini context
     * @return the section pointer, return NULL if the section not exist
     */
    const ini_section_t *ini_get_section(const char *section_name, 
            const ini_context_t *context);

    /**
     * @brief get item string value
     * @param section the section pointer, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @return item value, return NULL when the item not exist
     */
    const char *ini_get_str_value1(const ini_section_t *section, 
            const char *item_name);

    /**
     * @brief get item int value (32 bits)
     * @param section the section pointer, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param default_value the default value
     * @return item value, return default_value when the item not exist
     */
    int ini_get_int_value1(const ini_section_t *section, 
            const char *item_name, const int default_value);

    /**
     * @brief get item int64 value (64 bits)
     * @param section the section pointer, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param default_value the default value
     * @return int64 value, return default_value when the item not exist
     */
    int64_t ini_get_int64_value1(const ini_section_t *section, 
            const char *item_name, const int64_t default_value);

    /**
     * @brief get item boolean value
     * @param section the section pointer, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param default_value the default value
     * @return item boolean value, return default_value when the item not exist
     */
    bool ini_get_bool_value1(const ini_section_t *section, 
            const char *item_name, const bool default_value);

    /**
     * @brief get item double value
     * @param section the section pointer, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param default_value the default value
     * @return item value, return default_value when the item not exist
     */
    double ini_get_double_value1(const ini_section_t *section, 
            const char *item_name, const double default_value);

    /**
     * @brief get item string values
     * @param section the section pointer, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param values   string array to store the values
     * @param max_values max string array elements
     * @return item value count, return 0 when the item not exist
     */
    int ini_get_str_values1(const ini_section_t *section, const char *item_name, 
            char **values, const int max_values);

    /**
     * @brief get item string value array
     * @param section the section pointer, NULL or empty string for 
     *                      global section
     * @param item_name the item name
     * @param context   the ini context
     * @param found_count store the item value count
     * @return item value array, return NULL when the item not exist
     */
    const ini_item_t *ini_get_str_values_ex1(const ini_section_t *section, 
            const char *item_name, int *found_count);

    /**
     * @brief print all items
     * @param context  the ini context
     */
    void ini_print_items(const ini_context_t *context);

#ifdef __cplusplus
}
#endif

#endif

