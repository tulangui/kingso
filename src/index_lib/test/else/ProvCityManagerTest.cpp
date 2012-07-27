/******************************************************************
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : ProvCityManager.cpp 的测试程序， 使用 gtest框架
 *              测试case是从上到下执行的， 不可随意调整顺序
 *
 ******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <gtest/gtest.h>

#include "util/strfunc.h"
#include "index_lib/ProvCityManager.h"


using namespace index_lib;


const char * g_pcPath  = "/home/yewang/kingso/project/index_lib/trunk/rundata/provcity.txt";

#define     FOR_NUM       2999999                             // 循环次数


// 测试  getInstance 是否 会返回 NULL
TEST( ProvCityManagerTest, Test_getInstance_NULL )
{
    for (int i = 0; i < FOR_NUM; ++i)
    {
        ASSERT_TRUE( (NULL != ProvCityManager::getInstance()) ) << "null at index:" << i;
    }
}


// 测试  多次  getInstance 返回 的 指针 是否相同
TEST( ProvCityManagerTest, Test_getInstance_DIFF )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    for (int i = 0; i < FOR_NUM; ++i)
    {
        ASSERT_TRUE( (dmgr == ProvCityManager::getInstance()) ) << "diff at index:" << i;
    }
}



// 测试  加载数据文件 是否成功
TEST( ProvCityManagerTest, Test_load )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    ASSERT_TRUE( dmgr->load( g_pcPath ) );
}



// 测试 查询单个的   在编码表中存在的区划
TEST( ProvCityManagerTest, Test_seek_single_true )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    FILE * fp = fopen( g_pcPath, "rb" );

    if ( !fp )   ADD_FAILURE();

    ssize_t    read;
    char     * line = NULL;
    size_t     len  = 0;
    char     * colArr[65] = {0};

    ProvCityID pcId;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        // 循环 按照  tab 分出所有的col
        str_split_ex( line, '\t', colArr, sizeof(colArr) / sizeof(char *) );

        ASSERT_EQ( 0, dmgr->seek( colArr[ 0 ], &pcId ) );

        ASSERT_EQ( atoi( colArr[ 1 ] ), pcId.pc_stu.area_id );
        ASSERT_EQ( atoi( colArr[ 2 ] ), pcId.pc_stu.prov_id );
        ASSERT_EQ( atoi( colArr[ 3 ] ), pcId.pc_stu.city_id );
    }

    fclose(fp);
    if ( line ) free(line);
}




// 测试 查询单个的   在编码表中存在的区划, 把区划带上 后缀
TEST( ProvCityManagerTest, Test_seek_single_true_suffix )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    ProvCityID pcId;

    ASSERT_EQ( 0, dmgr->seek( "浙江省", &pcId ) );
    ASSERT_EQ( 0, dmgr->seek( "江苏省", &pcId ) );
    ASSERT_EQ( 0, dmgr->seek( "杭州市", &pcId ) );
    ASSERT_EQ( 0, dmgr->seek( "北京市", &pcId ) );
}





// 测试 查询单个的   在编码表中  不存在的区划
TEST( ProvCityManagerTest, Test_seek_single_false )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    ProvCityID pcId;

    ASSERT_EQ( -1, dmgr->seek( "aaa", &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "江江江江江江江江江江江江江江江江江江江江江", &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "杭市", &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "122211111", &pcId ) );
}


// 测试 查询单个的   在编码表中  不存在的区划   带后缀
TEST( ProvCityManagerTest, Test_seek_single_false_suffix )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    ProvCityID pcId;

    ASSERT_EQ( -1, dmgr->seek( "省", &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "市", &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "杭市", &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "122211111省", &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "啊发发的发生地发放爱的市", &pcId ) );
}






// 测试 查询组合的   在编码表中存在的区划
TEST( ProvCityManagerTest, Test_seek_true )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    ProvCityID pcId;

    ASSERT_EQ( 0 , dmgr->seek( "浙江", ' ', &pcId ) );
    ASSERT_EQ( 0 , dmgr->seek( "杭州", ' ', &pcId ) );
    ASSERT_EQ( 0 , dmgr->seek( "浙江 杭州", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "杭州 浙江", ' ', &pcId ) );

    ASSERT_EQ( 0 , dmgr->seek( "港澳台", ' ', &pcId ) );
    ASSERT_EQ( 0 , dmgr->seek( "香港", ' ', &pcId ) );
    ASSERT_EQ( 0 , dmgr->seek( "港澳台 香港", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "香港 港澳台", ' ', &pcId ) );
}





// 测试 查询组合的   在编码表中存在的区划
TEST( ProvCityManagerTest, Test_seek_true_suffix )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    ProvCityID pcId;

    ASSERT_EQ( 0 , dmgr->seek( "浙江省", ' ', &pcId ) );
    ASSERT_EQ( 0 , dmgr->seek( "杭州市", ' ', &pcId ) );
    ASSERT_EQ( 0 , dmgr->seek( "浙江省 杭州市", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "杭州市 浙江省", ' ', &pcId ) );
}




// 测试 查询组合的   在编码表中 不 存在的区划
TEST( ProvCityManagerTest, Test_seek_false )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    ProvCityID pcId;

    ASSERT_EQ( -1, dmgr->seek( "aaa", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "bbbbbbbbbbbbbbb", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "aaa 122", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "9282- 1615155", ' ', &pcId ) );
}




// 测试 查询组合的   在编码表中存在的区划
TEST( ProvCityManagerTest, Test_seek_false_suffix )
{
    ProvCityManager * dmgr = ProvCityManager::getInstance();

    ProvCityID pcId;

    ASSERT_EQ( -1, dmgr->seek( "dd省", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "ff市", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "1111111111111111111省 ccccccccccccccccccccc市", ' ', &pcId ) );
    ASSERT_EQ( -1, dmgr->seek( "afafafafasf市 啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊省", ' ', &pcId ) );
}



// 主测试入口
int main(int argc, char * argv[])
{
    alog::Configurator::configureRootLogger();

     ::testing::InitGoogleTest(&argc, argv);

     return RUN_ALL_TESTS();
}
