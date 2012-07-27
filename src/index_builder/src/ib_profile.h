/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ib_profile.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_IB_PROFILE_H_
#define SRC_IB_PROFILE_H_

#include <stdint.h>
#include <zlib.h>

#include "ib_ctx.h"
#include "ks_build/ks_build.h"

int ib_write_profile(gzFile fp, ksb_conf_t *conf, ksb_result_t *presult);

#endif //SRC_IB_PROFILE_H_
