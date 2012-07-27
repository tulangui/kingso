/*********************************************************************
 * $Author: pianqian.gc $
 *
 * $LastChangedBy: pianqian.gc $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: netdef.h 2577 2011-03-09 01:50:55Z pianqian.gc $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef _NETDEF_H_
#define _NETDEF_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

typedef struct
{
    uint64_t seq;
    uint8_t cmd;   // add、mod、del、 处理结果
    uint32_t size;
    uint8_t cmpr;  // 压缩标志 非压缩: N 压缩: Z
}
net_head_t;

typedef enum
{
    UPCMD_REQ_ADD = 0x01,
    UPCMD_REQ_MOD = 0x02,
    UPCMD_REQ_DEL = 0x03,

    UPCMD_ACK_SUCCESS       = 0x80, //正确结果
    UPCMD_ACK_EUNKOWN       = 0x81, //未知错误
    UPCMD_ACK_EFORMAT       = 0x82, //格式错误   
    UPCMD_ACK_EUNRECOGNIZED = 0x83, //无法识别的命令
    UPCMD_ACK_EPROCESS      = 0x84, //处理错误
}
net_cmd_t;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif //_NETDEF_H_
