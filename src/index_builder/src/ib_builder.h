/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ib_builder.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_IB_BUILDER_H_
#define SRC_IB_BUILDER_H_

#include "ks_build/ks_build.h"
#include "ib_ctx.h"
#include "xml_parser.h"

typedef struct ib_builder_ctx_s{
    ksb_conf_t *conf;
    ib_ctx_t *p_ibctx;
}ib_builder_ctx_t;

int ib_build(ib_builder_ctx_t *pctx);

#endif //SRC_IB_BUILDER_H_
