/** \file
 *******************************************************************
 * $Author: gongyi.cl $
 *
 * $LastChangedBy: gongyi.cl $
 *
 * $Revision: 1042 $
 *
 * $LastChangedDate: 2011-04-21 14:00:11 +0800 (Thu, 21 Apr 2011) $
 *
 * $Id: filefunc.h 1042 2011-04-21 06:00:11Z gongyi.cl $
 *
 * $Brief: file functions $
 *******************************************************************
 */

#ifndef FILE_FUNC_H
#define FILE_FUNC_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief check file exist
     * @param	filename the filename to check
     * @return true if file exists, otherwise false
     */
    bool file_exist(const char *filename);

    /**
     * @brief get file content
     * @param filename the filename
     * @param buff return the buff, buffer last set to '\\0', the buff must be freed
     * @param file_size store the file size
     * @return error no , 0 success, != 0 fail
     */
    int file_get_content(const char *filename, char **buff, int64_t *file_size);

    /**
     * @brief write buffer to file
     * @param filename the filename to write
     * @param buff the buffer to write
     * @param file_size the file size
     * @return error no , 0 success, != 0 fail
     */
    int file_write_buffer(const char *filename, const char *buff, 
            const int file_size);
    
    char * get_realpath(const char * path, char * rpath);
    int make_dir(const char *patchname, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif // FILE_FUNC_H

