/*********************************************************************
 * $Author: pianqian.gc $
 *
 * $LastChangedBy: pianqian.gc $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: conhash_code.h 2577 2011-03-09 01:50:55Z pianqian.gc $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef _CONHASH_CODE_H_
#define _CONHASH_CODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* 当存在1~8个节点时的虚节点个数 */
extern unsigned int vnode_count[9];

/* 当存在1~8个节点时，节点在虚节点中的分布 */
extern unsigned char vnode_map[9][65536];

/* 当存在1~8个节点时，哈希值与虚节点的对应关系 */
extern unsigned int conhash_map[9][65536];

#ifdef __cplusplus
}
#endif

#endif //_CONHASH_CODE_H_
