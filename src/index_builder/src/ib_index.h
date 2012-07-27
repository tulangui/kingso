/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ib_index.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_IB_INDEX_H_
#define SRC_IB_INDEX_H_

#include <stdint.h>
#include <zlib.h>

#include "ib_ctx.h"
#include "ks_build/ks_build.h"
#pragma pack (1)
typedef struct ib_text_s{
    uint64_t sign;
    uint32_t occ;
}ib_text_t;

typedef struct ib_text_sn_s{
    ib_text_t node;
    uint32_t docid;
}ib_text_sn_t;

typedef struct ib_num_s{
    uint64_t sign;
}ib_num_t;

typedef struct ib_num_sn_s{
    ib_num_t node;
    uint32_t docid;
}ib_num_sn_t;

#pragma pack ()

int ib_write_index(gzFile *fps, ksb_conf_t *conf, ksb_result_t *presult);



#endif //SRC_IB_INDEX_H_
