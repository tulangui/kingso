
/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ksb_ws.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_KSB_WS_H_
#define SRC_KSB_WS_H_

#include "ks_build/ks_build.h"


int ksb_do_ws(const char *str, ksb_result_field_t * pfield,
              ksb_conf_t * conf, ksb_result_field_t *pfield_bind_to, uint32_t bind_occ, uint32_t bind_real_occ, int index);

int ksb_insert_result(ksb_result_field_t * pfield, uint64_t sign,
                      uint32_t occ, uint32_t weight);
void ksb_ws_sort_uniq(ksb_result_field_t * pfield);

#endif                          //SRC_KSB_WS_H_
