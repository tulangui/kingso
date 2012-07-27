#ifndef TS_INDEX_STRUCT_H
#define TS_INDEX_STRUCT_H

#include <limits.h>
#include <unistd.h>
#include <stdint.h>

#include "idx_dict.h"
#include "index_lib/IndexLib.h"

// macros defined here
//
#define INVERTED_IDX1_INFIX           "term"
#define INVERTED_IDX2_INFIX           "index"
#define BITMAP_IDX1_INFIX             "bitmap_term"
#define BITMAP_IDX2_INFIX             "bitmap_index"
#define INC_IND2_POSTFIX              "inc_index"
#define INC_IND1_INFIX                "inc_term"
#define INDEX_GLOBAL_INFO             "index.ginfo"
#define INDEX_MEMPOOL_DIR             "mempool"

const uint32_t MAX_IDX2_FD_BIT        = 5;                         // 二级倒排索引文件个数最大占用bit数
const uint32_t MAX_IDX2_NUM_BIT       = 26;                        // 2级索引链条中doc的数量占用的bit数
const uint32_t MAX_IDX2_FD            = (1<<MAX_IDX2_FD_BIT);      // 2级倒排索引文件的最多个数
const uint32_t MAX_IDX2_DOC_NUM       = (1<<MAX_IDX2_NUM_BIT);     // 2级索引链条中doc的最大数量
const uint32_t MAX_IDX2_FILE_LEN      = 1L<<30;                    // 1G，2级倒排索引文件的近似大小

const uint32_t MAX_INC_SKIP_CNT       = 128;                       // 增量状态下zip格式倒排的skipblock最大数
const uint32_t MAX_INC_BITMAP_SIZE    = 1048568;                   // 增量bitmap最大空间 1M (差8字节用于结尾保护)

// 全量建库结构体
typedef struct _index_builder_t {
	char            path[PATH_MAX];              // 索引文件所在目录
	char            field_name[PATH_MAX];        // 索引字段名

	idx_dict_t    * inverted_dict;               // 1级倒排索引hash表
	idx_dict_t    * bitmap_dict;                 // 1级bitmap索引hash表

	int             inverted_fd[MAX_IDX2_FD];    // 2级倒排索引句柄数组
	uint32_t        inverted_fd_num;             // 当前已打开的2级倒排索引句柄个数
	uint32_t        cur_size;                    // 当前倒排文件的已写入长度

	int             bitmap_fd;                   // 2级bitmap索引句柄
	uint32_t        bitmap_size;

	char          * raw_buf;                     // 存储原始2进制索引的buffer
	uint32_t        raw_buf_size;                // raw buffer长度
	char          * final_buf;                   // 存储最终2进制索引的buffer
	uint32_t        final_buf_size;              // final buffer长度
} index_builder_t;


#pragma pack (1)
// 全量索引的1级索引单元      (备注: 这个结构是要放到 pdict里面去的，dict的node元素有next成员，在这里隐藏了 )
typedef struct _full_idx1_unit_t {
    uint64_t      term_sign;                     // term的64位签名 (key)
    uint32_t      pos;                           // 2级索引链条在文件中的偏移
    uint32_t      len:30;                        // 2级索引链条的长度
    uint32_t      zip_flag:2;                    // 压缩类型，0:没有倒排链; 1: 不压缩; 2: 高低位压缩; 3: p4delta压缩
    uint32_t      file_num:MAX_IDX2_FD_BIT;      // 2级索引链条所在文件编号
    uint32_t      doc_count:MAX_IDX2_NUM_BIT;    // 2级索引链条中doc的数量
    uint32_t      not_orig_flag:1;               // 是否是重新打开所后追加的token的标记，0表示不是， 1表示是
} full_idx1_unit_t;

// 全量索引的1级索引bitmap索引单元
typedef struct _bitmap_idx1_unit_t {
    uint64_t      term_sign;                     // term的64位签名 (key)
    uint32_t      pos;                           // 2级索引链条在文件中的偏移
    uint32_t      max_docId;                     // 2级索引链条中最大的docId
    uint32_t      doc_count:MAX_IDX2_NUM_BIT;    // 2级索引链条中doc的数量
    uint32_t      not_orig_flag:1;               // 是否是重新打开所后追加的token的标记，0表示不是， 1表示是
} bitmap_idx1_unit_t;


// 增量索引的1级索引单元
typedef struct _inc_idx1_unit_t {
    uint64_t      term_sign;                     // term的64位签名 (key)
    uint32_t      doc_count;                     // 2级索引链条中doc的数量
    uint32_t      pos;                           // 2级索引链条在文件中的偏移
} inc_idx1_unit_t;


typedef struct _inc_idx2_header_t {
    uint32_t      pageSize;                      // block size
    uint32_t      endOff;                        // 最后使用偏移
    uint8_t       zipFlag;                       // 区分压缩非压缩标识
} inc_idx2_header_t;

// 2级未压缩索引单元
typedef struct _idx2_nzip_unit_t {
    uint32_t      doc_id;                        // doc-id
    uint8_t       occ;                           // term在该doc中的第一次出现的位置
} idx2_nzip_unit_t;

// 2级压缩索引的header结构体
typedef struct _idx2_zip_header_t {
    uint32_t      skip_list_len;                 // SkipList长度
    uint32_t      doc_list_len;                  // docList长度
    uint32_t      block_num;                     // 压缩块的个数, 即skipList中元素的个数
} idx2_zip_header_t;

// 2级压缩索引SkipList单元
typedef struct _idx2_zip_skip_unit_t {
    uint16_t      doc_id_base;                   // doc-id的高10bit
    uint32_t      off;                           // docList中的偏移（以idx2_zip_list_unit_t为单位）
} idx2_zip_skip_unit_t;

// 2级压缩索引DocList单元
typedef struct _idx2_zip_list_unit_t {
    uint16_t      doc_id_diff;                   // doc-id的低16bit
    uint8_t       occ;                           // term在该doc中的第一次出现的位置
} idx2_zip_list_unit_t;


// 2级p4delta压缩索引SkipList单元
typedef struct _idx2_p4d_skip_unit_t {
    uint32_t      doc_id_base;  // block中最大docid
    uint32_t      off;                        // docList中的偏移（以byte为单位）
} idx2_p4d_skip_unit_t;

#pragma pack ()


const uint64_t bit_mask_tab[64]={
	1L<<0,  1L<<1,  1L<<2,  1L<<3,  1L<<4,  1L<<5,  1L<<6,  1L<<7,
	1L<<8,  1L<<9,  1L<<10, 1L<<11, 1L<<12, 1L<<13, 1L<<14, 1L<<15,
	1L<<16, 1L<<17, 1L<<18, 1L<<19, 1L<<20, 1L<<21, 1L<<22, 1L<<23,
	1L<<24, 1L<<25, 1L<<26, 1L<<27, 1L<<28, 1L<<29, 1L<<30, 1L<<31,
	1L<<32, 1L<<33, 1L<<34, 1L<<35, 1L<<36, 1L<<37, 1L<<38, 1L<<39,
	1L<<40, 1L<<41, 1L<<42, 1L<<43, 1L<<44, 1L<<45, 1L<<46, 1L<<47,
	1L<<48, 1L<<49, 1L<<50, 1L<<51, 1L<<52, 1L<<53, 1L<<54, 1L<<55,
	1L<<56, 1L<<57, 1L<<58, 1L<<59, 1L<<60, 1L<<61, 1L<<62, 1L<<63
};

const uint64_t bit_mask_tab2[64]={
	~(1L<<0),  ~(1L<<1),  ~(1L<<2),  ~(1L<<3),  ~(1L<<4),  ~(1L<<5),  ~(1L<<6),  ~(1L<<7),
	~(1L<<8),  ~(1L<<9),  ~(1L<<10), ~(1L<<11), ~(1L<<12), ~(1L<<13), ~(1L<<14), ~(1L<<15),
	~(1L<<16), ~(1L<<17), ~(1L<<18), ~(1L<<19), ~(1L<<20), ~(1L<<21), ~(1L<<22), ~(1L<<23),
	~(1L<<24), ~(1L<<25), ~(1L<<26), ~(1L<<27), ~(1L<<28), ~(1L<<29), ~(1L<<30), ~(1L<<31),
	~(1L<<32), ~(1L<<33), ~(1L<<34), ~(1L<<35), ~(1L<<36), ~(1L<<37), ~(1L<<38), ~(1L<<39),
	~(1L<<40), ~(1L<<41), ~(1L<<42), ~(1L<<43), ~(1L<<44), ~(1L<<45), ~(1L<<46), ~(1L<<47),
	~(1L<<48), ~(1L<<49), ~(1L<<50), ~(1L<<51), ~(1L<<52), ~(1L<<53), ~(1L<<54), ~(1L<<55),
	~(1L<<56), ~(1L<<57), ~(1L<<58), ~(1L<<59), ~(1L<<60), ~(1L<<61), ~(1L<<62), ~(1L<<63)
};

#endif
