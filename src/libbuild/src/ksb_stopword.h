
/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ksb_stopword.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_KSB_STOPWORD_H_
#define SRC_KSB_STOPWORD_H_

#include "ks_build/ks_build.h"

int ksb_load_stopword(const char *stopword_dict, ksb_conf_t * conf);

inline int
ksb_is_stopword(int word, ksb_conf_t * conf) {
    if (word >= 65536 || word < 0) {
        return 0;
    }
    int pos = word / 64;
    int bit = word % 64;
    return (conf->stopword->map[pos] & (1 << bit)) ? 1 : 0;
}

#endif                          //SRC_KSB_STOPWORD_H_
