/** \file
 *******************************************************************
 * $Author: pianqian $
 *
 * $LastChangedBy: pianqian $
 *
 * $LastChangedDate: 2011-09-20 16:10:54 +0800 (Tue, 20 sep 2011) $
 *
 * $Brief: net protocol package of kingso $
 *******************************************************************
 */

#ifndef _KS_PACK_H_
#define _KS_PACK_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * kspack数据类型
 */
enum 
{ 
    KS_NONE_VALUE, // 没有数据
    KS_BYTE_VALUE, // 二进制数据
    KS_PACK_VALUE, // 子数据包类型
    KS_VALUE_SIZE, // 数据类型数量
};

/*
 * kspack类型，具体定义在.c文件中
 */
typedef struct _kspack kspack_t;

/*
 * 打开一个kspack
 *
 * rw   : 以何种形式打开。'r'以读方式打开，'w'以写方式打开
 * data : 数据内存。如果以读方式打开必须包含该参数；如果以写方式打开并包含该参数，只能向该存储区域内写入数据
 * size : 数据长度。
 *
 * return : kspack实体；错误返回NULL
 */
extern kspack_t* kspack_open(const char rw, void* data, const int size);

/*
 * 在一个kspack内部打开一个子kspack
 *
 * parent : 上层kspack
 * name : 子kspack在上层kspack中的节点名字
 * nlen : 名字长度
 *
 * return : kspack实体；错误返回NULL
 */
extern kspack_t* kspack_sub_open(kspack_t* parent, const char* name, const int nlen);

/*
 * 关闭kspack实例
 *
 * pack : 被关闭的实例
 * dist : 是否清理数据包缓冲区
 */
extern int kspack_close(kspack_t* pack, int dist);

/*
 * kspack写入或读取结束
 *
 * pack : 已结束的实例
 *
 * return : 正确返回0；错误返回-1
 */
extern int kspack_done(kspack_t* pack);

/*
 * 返回一个kspack实例对应的内存区域
 *
 * pack : kspack实例
 *
 * return : 内存区域；未关闭返回NULL
 */
extern void* kspack_pack(kspack_t* pack);

/*
 * 返回一个kspack实例对应的内存区域长度
 *
 * pack : kspack实例
 *
 * return : 内存区域长度；未关闭返回-1
 */
extern int kspack_size(kspack_t* pack);

/*
 * 向一个kspack实例中添加数据
 *
 * pack  : kspack实例
 * name  : 数据名字
 * nlen  : 名字长度
 * value : 数据内容
 * vlen  : 内容长度
 * vtype : 数据类型
 *
 * return : 添加成功返回0；不成功返回-1
 */
extern int kspack_put(kspack_t* pack, const char* name, const int nlen, const void* value, const int vlen, const char vtype);

/*
 * 从一个kspack实例中获取数据
 *
 * pack  : kspack实例
 * name  : 数据名字
 * nlen  : 名字长度
 * value : 用于指向数据内容
 * vlen  : 用于保存内容长度
 * vtype : 用于保存数据类型
 *
 * return : 获取成功返回0；不成功返回-1
 */
extern int kspack_get(kspack_t* pack, const char* name, const int nlen, void** value, int* vlen, char* vtype);

/*
 * 初始化轮询游标
 *
 * pack  : kspack实
 */
extern void kspack_first(kspack_t* pack);

/*
 * 从kspack实例中获取下一条数据
 *
 * pack  : kspack实例
 * name  : 数据名字
 * nlen  : 名字长度
 * value : 用于指向数据内容
 * vlen  : 用于保存内容长度
 * vtype : 用于保存数据类型
 *
 * return : 获取成功返回0；全部数据都已获取返回-1
 */
extern int kspack_next(kspack_t* pack, char** name, int* nlen, void** value, int* vlen, char* vtype);

#ifdef __cplusplus
}
#endif

#endif
