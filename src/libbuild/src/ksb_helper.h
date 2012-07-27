
/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ksb_helper.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_KSB_HELPER_H_
#define SRC_KSB_HELPER_H_
#include <stdio.h>
#include <stdint.h>


uint64_t get_nick_hash(const char *gbk_nick);

int ksb_normalize(char *str, int len);

#endif                          //SRC_KSB_HELPER_H_
