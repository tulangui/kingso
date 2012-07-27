/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ProfileStruct.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: structure definition of profile $
 ********************************************************************/
#ifndef KINGSO_INDEXLIB_PROFILE_STRUCT_H
#define KINGSO_INDEXLIB_PROFILE_STRUCT_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "IndexLib.h"

namespace index_lib 
{

class ProfileMMapFile;
class ProfileEncodeFile;
class ProfileSyncManager;

/**
 *  字段存储枚举类型定义
 */
typedef enum {
    MT_ONLY,           //字段值直接存储在主表中
    MT_ENCODE,         //字段值存储在主表 + 编码表中
    MT_BIT,            //字段值存储在主表 + bit编码中
    MT_BIT_ENCODE      //字段值存储子主表 + bit编码 + 编码表中
}STORE_TYPE;

/**
 * KV32结构体定义
 */
typedef struct _profile_kv32 {
        int32_t key;
            int32_t value;
}KV32;

/**
 * KV32转换为int64类型
 */
inline int64_t ConvFromKV32(KV32 kv)
{
    union {
        int64_t lvalue;
        KV32    kvalue;
    };

    kvalue = kv;
    return lvalue;
}

/**
 *  不同类型的字段空值结构定义
 */
typedef union  _union_empty_value {
    int8_t   EV_INT8;
    int16_t  EV_INT16;
    int32_t  EV_INT32;
    int64_t  EV_INT64;
    uint8_t  EV_UINT8;
    uint16_t EV_UINT16;
    uint32_t EV_UINT32;
    uint64_t EV_UINT64;
    float    EV_FLOAT;
    double   EV_DOUBLE;
    char*    EV_STRING;
    KV32     EV_KV32;
}EmptyValue;

/**
 * AttributeGroupResource结构体定义
 */
typedef struct _attribute_group_resource {
    uint32_t         segNum : 7,                            //当前的分段个数
                     unitSpace;                             //当前属性分组的存储单元空间大小
    ProfileMMapFile *mmapFileArr  [MAX_SEGMENT_NUM];        //属性分组对应的MMapFile资源数组
    char            *segAddressArr[MAX_SEGMENT_NUM];        //分段文件对应的起始地址数组
}AttributeGroupResource;

/**
 * ProfileResource结构体定义
 */
typedef struct _profile_resource {
    uint32_t                 docNum;                          //profile当前的最大DocID
    uint32_t                 groupNum;                        //当前的属性分组个数
    AttributeGroupResource * groupArr[MAX_PROFILE_FIELD_NUM]; //属性分组资源对应的数组
    ProfileSyncManager     * syncMgr;                         //profile同步管理器对象
}ProfileResource;

/**
 * BitRecordDescriptor结构体定义
 */
typedef struct _bitrecord_descriptor {
    uint32_t field_len : 5;        //bit字段长度（位）：1~31  
    uint32_t u32_offset: 8;        //bit字段在buffer中的起始偏移量：0~255(偏移量单位为4byte)
    uint32_t bits_move : 5;        //bit字段在读写过程中用到（左移/右移)的位数：0~31
    uint32_t read_mask;            //bit字段在读取时的对应掩码：对应位的值为1，与write_mask按位取反
    uint32_t write_mask;           //bit字段在写入时的对应掩码：对应为的值为0，与read_mask按位取反
}BitRecordDescriptor;

/**
 * ProfileField结构体定义
 */
struct ProfileField {
    char                  name [MAX_FIELD_NAME_LEN];  //字段名称
    char                  alias[MAX_FIELD_NAME_LEN];  //别名字段名称
    PF_DATA_TYPE          type;                       //字段类型
    PF_FIELD_FLAG         flag;                       //字段flag
    uint32_t              multiValNum : 10,           //字段多值个数, 0:多值变长  1:单值定长 n:多值定长
                          offset      : 12,           //字段对应在属性群组内的偏移量(0~2K),最大偏移量为256*8
                          unitLen     : 10;           //主表内记录的基本类型长度最大为8byte,最大为uint32数组(bitRecord)
    uint32_t              groupID;                    //字段所在的群组ID,0~255
    bool                  isBitRecord;                //是否是bitRecord压缩字段
    bool                  isEncoded;                  //是否是编码字段
    bool                  isCompress;                 //是否对字段值进行压缩
    BitRecordDescriptor  *bitRecordPtr;               //bitRecord字段描述指针，如果为bitRecord字段则不为NULL
    ProfileEncodeFile    *encodeFilePtr;              //编码表对象指针，如果为编码表字段则不为NULL
    STORE_TYPE            storeType;                  //字段底层存储类型
    EmptyValue            defaultEmpty;               //字段对应的默认空值

public:
    PF_DATA_TYPE getAppType() const {
        if (unlikely(flag == F_LATLNG_DIST)) {
            return DT_DOUBLE;
        }
        return type;
    }
};

}

#endif //KINGSO_INDEXLIB_PROFILE_STRUCT_H
