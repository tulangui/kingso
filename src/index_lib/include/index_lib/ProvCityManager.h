/******************************************************************
 * ProvCityManager.h
 *
 *      Author: yewang@taobao.com clm971910@gmail.com
 *      Create: 2011-1-13
 *
 *      Desc  : 提供从 区划编码表 文件 的加载功能
 *              提供从 地址 到 编码的查询 接口
 *              提供编码的比对接口
 *
 *  1. 应用背景
 *     用户根据卖家所在地， 选择卖家。  用户可能输入： 江浙沪  海外  美国  浙江  杭州， 最多到市这一级。
 *  2. 编码方案
 *     我们采用 3级编码， 共 2个Byte来 存储所有的信息， 每个宝贝 将获得一个 唯一的卖家所在地编号
 *     具体是:
 *        一 级编码   5个bit    用来存区域  ( 江浙沪,  海外)
 *        二 级编码   6个bit    用来存省份   ( 浙江 ,   日韩)
 *        三 级编码   5个bit    用来存市       ( 杭州 ,   韩国)
 *     编码样例:
 *        江浙沪            1    0   0               后2个编号为  0
 *        浙江                1    1   0               第一个编号和  江浙沪相同， 最后一个编号 为 0
 *        杭州                1    1   1               前2个编号 和 浙江相同
 *        这样我们通过编码就可以确定， 各个层级之间才从属关系
 *        如果省和市存在重名的情况， 我们统一确定为  市级单位需要带上自己的区划后缀
 *
 *        第一级编码  和   三级编码     有 从属关系
 *        第 二级编码  和   三级编码    有 从属关系
 *        第一级编码  和   二级编码     没有   从属关系
 *
 *  3. Build数据
 *     在xml数据里面的 provcity 字段， 数据可能为  ：  “浙江”   “浙江  杭州” “深圳”  (空格分隔的)
 *     在buid时， 如果只有1个层级输入。
 *         比如:  “浙江”  “深圳”
 *         通过查询编码表  取得  对应的 id ， 存入profile的字段中
 *
 *     在buid时， 如果有2个层级输入。
 *         比如:   “浙江  杭州”    “海外   美国”
 *         先查询  “浙江”  “海外”  获得对应的编码
 *         然后再查询  “杭州” “美国”， 获得对应的编码。
 *
 *         根据从属关系  判断  是否正确 。    如果不对 ： 报错
 *         如果对， 我们用查询  “杭州” “美国”， 获得的编码
 *
 *  4. 用户查询, 过滤宝贝
 *     用户可能输入： "江浙沪"  "海外"   "美国"   "浙江"  "杭州"
 *     1.  根据用户的输入，    查表 取得对应的编码号。
 *     2.  判断编码的的层级， 如果  后11个bit为  0 ， 则为区域一级（江浙沪、海外）。
 *           在过滤时， 和 每个宝贝的provcity字段的前5个bit比较。 不同 就删除
 *
 *     3.  判断编码的的层级， 如果  后5个bit为  0, 前5个bit为0 ， 则为省一级（浙江、欧美），
 *           在过滤时， 和 每个宝贝的provcity字段的中间6个bit比较。 不同 就删除
 *
 *     4. 如果  后5个bit不为  0， 则为 最细层级。  （杭州   美国）
 *           在过滤时，直接比较 整个编号， 不同就删除
 *
 *  5. 优点
 *     1.  省内存，  5000万宝贝， 这个字段 只需要 用  100M 内存
 *     2.  检索快，  只需要扫一次内存， 每个宝贝执行一次比较   就OK了
 ******************************************************************/



#ifndef PROVCITYMANAGER_H_
#define PROVCITYMANAGER_H_


#include "commdef/commdef.h"
#include "IndexLib.h"
#include "util/hashmap.h"





namespace index_lib
{
#pragma pack (1)
/**
 * 区划编码ID结构体,  刚好 16个bit。  共  2 Byte
 */
union ProvCityID
{
    struct
    {
        uint16_t   area_id:5;                    // 区域id, 江浙沪
        uint16_t   prov_id:6;                    // 省份id
        uint16_t   city_id:5;                    // 城市id
    } pc_stu;

    uint16_t       pc_id;                        // 完整的编号
};
#pragma pack ()



class ProvCityManager
{
public:

    /**
     * 获取单实例对象指针, 实现类的单例
     *
     * @return  NULL: error
     */
    static ProvCityManager * getInstance();


    /**
     * 释放类的单例
     */
    static void freeInstance();


    /**
     * 载入区划编码编码数据文件。
     *
     * @param  filePath 数据存放的完整路径
     *
     * @return  false: 载入失败;  true: 载入化成功
     */
    bool  load(const char * filePath);



    /**
     * 根据  单个区划名称 查询 对应的编码
     *
     * @param    name      输入的区划名字   "杭州"
     * @param    pcId      输出的编码
     *
     * @return  -1: 不存在这个区划；   0: 转换成功
     */
    int  seek(const char * name, ProvCityID * pcId);



    /**
     * 根据的  区划名称 查询 对应的编码, 可能是组合式的
     *
     * @param name        输入的区划名字   "杭州"  or "浙江 杭州"
     * @param delim       分隔符
     * @param pcId        输出的编码
     *
     * @return  -1: 不存在这个区划；   0: 转换成功
     */
    int  seek(const char * name, char delim, ProvCityID * pcId);



    /**
     * 重置内部数据， 用于测试
     */
    void reset();





private:
    // 构造函数、析构函数
    ProvCityManager();
    virtual ~ProvCityManager();


    /**
     * 载入区划编码编码数据文件。
     *
     * @param fp        文件句柄
     *
     * @return false: 载入失败;  true: 载入化成功
     */
    bool  trueLoad(FILE * fp);



    /**
     * 销毁运行中分配的资源
     *
     * @return
     */
    bool destroy();


private:
    static ProvCityManager   * instance;         // 类的惟一实例


private:
    hashmap_t    _dict;                          // 行政区划  词典


};







}

#endif /* PROVCITYMANAGER_H_ */
