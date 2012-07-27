/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: xml_parser.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_XML_PARSER_H_
#define SRC_XML_PARSER_H_

#include <pthread.h>
#include <zlib.h>
#include "ks_build/ks_build.h"
#include "ib_ctx.h"

typedef struct xp_ctx_s{
    ib_ctx_t *p_ibctx;
    uint32_t doc_count;
    int ret_val;

    pthread_t tid;
    uint32_t id;
    uint32_t fcnt;
    ksb_conf_t *conf;
    gzFile fp_profile;
    gzFile fp_index[KSB_FIELD_MAX*INDEX_SPLIT_PARTS];
    gzFile fp_detail_idx;
    gzFile fp_detail;
}xp_ctx_t;

extern int xp_load_input_queue(ib_ctx_t *p_ibctx);
extern void xp_cleanup_input_queue();
extern void *xp_parse_thread(void *pdata);
extern int xp_open_output_files(xp_ctx_t *pctx,int is_gzip);
extern void xp_close_output_files(xp_ctx_t *pctx);


#endif //SRC_XML_PARSER_H_
