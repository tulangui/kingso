/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ib_ctx.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_IB_CTX_H_
#define SRC_IB_CTX_H_

#include <stdint.h>

#define EXT_GZIP ".gz"
#define EXT_XML  ".xml"


#define INDEX_FILENAME "index.%s.%3.3u.%3.3u"
#define PROFILE_FILENAME "profile.%3.3u"
#define DETAIL_IDX_FILENAME "detail.idx.%3.3u"
#define DETAIL_FILENAME "detail.dat.%3.3u"

#define INDEX_FILENAME_BP "index.%s.%%3.3u.%%3.3u"
#define PROFILE_FILENAME_BP PROFILE_FILENAME

#define TEXT_OCC_COUNT 1
#define INDEX_SPLIT_PARTS 10


typedef struct ib_ctx_s{
    const char *input_dir;
    const char *output_dir;
    const char *tmp_dir;
    const char *conf_file;
    const char *file_ext;
    int32_t is_gzip;
    int32_t is_fixseq;
    uint32_t mode;

    uint32_t thread_count;

    uint32_t doc_total;
}ib_ctx_t;

#endif //SRC_IB_CTX_H_
