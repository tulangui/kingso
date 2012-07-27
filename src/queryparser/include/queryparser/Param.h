/***********************************************************************************
 * Describe : queryparser输出的key-value结构的虚基类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-07
 **********************************************************************************/


#ifndef _QP_CLASS_PARAM_H_
#define _QP_CLASS_PARAM_H_


#include "util/HashMap.hpp"
#include "commdef/commdef.h"
#include "util/MemPool.h"


#define QUERY_FLAG "#@QUERY@#"
#define HLKEY_FLAG "#@HLKEY@#"

namespace queryparser {
class Param
{
public :

    Param(MemPool *mempool);
    virtual ~Param(); 

    /*
     * 检查某个关键字是否存在
     *
     * @param key 关键字
     *
     * @return true  该关键字存在
     *         false 该关键字不存在
     */
    bool hasParam(const char* key) const ;

    /*
     * 获取某个关键字对应的数据
     *
     * @param key 关键字
     *
     * @return NULL 该关键字不存在
     *         else 该关键字对应的数据
     */
    const char* getParam(const char* key) const;

    /*
     * 获取客户端经纬度位置关键字对应的数据
     *
     * @param lat [out] 纬度
     * @param lng [out] 经度
     *
     * @return true, 字段存在且解析有效;
     *         false, 字段不存在，或格式无效
     */
    bool getLatLngParam(float & lat, float & lng) const;
    

    void setQueryString(const char* queryString, int queryLength);
    void setHlkeyString(const char* hlkeyString, int hlkeyLength, bool encode = false);
    /*
     * 修改/添加某个关键字
     *
     * @param key 关键字
     * @param value 关键字对应的值
     *
     * @return true  修改/添加成功
     *         false 修改/添加失败
     */
    bool setParam(char* key, char* value);

    /**
     * 获取节点个数
     *
     * @return < 0 执行错误
     *         >=0 节点个数
     */
    const int getNum() const;

    /*
     * 使游标指向第一个数据
     */
    void first();

    /*
     * 获取下一组数据
     *
     * @param key 将要指向关键字
     * @param value 将要指向数据
     *
     * @return < 0 遍历完成
     *         ==0 正确返回结果
     */
    int next(char* &key, int *key_len,  char* &value, int *value_len);
    
    void print();
private:	
    
    util::HashMap<const char*, char *> _dict;                      // key-value对字典实体
    util::HashMap<const char*, char *>::iterator _iter; 
    bool mDestroy;                         // key-value对字典是否由该类销毁

    char mQueryString[MAX_QYERY_LEN+1];    // 查询条件字符串
    int mQueryLength;                      // 查询条件字符串长度

    char mHlkeyString[MAX_QYERY_LEN+1];    // 高亮字符串
    int mHlkeyLength;                      // 高亮字符串长度
    MemPool *_mempool;
};

}

#endif
