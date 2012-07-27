/******************************************************************
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : DocIdManger.cpp 的测试程序， 使用 gtest框架
 *              测试case是从上到下执行的， 不可随意调整顺序
 *
 ******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <gtest/gtest.h>

#include "index_lib/DocIdManager.h"


using namespace index_lib;


#define     FOR_NUM       2999999                             // 循环次数

const char * g_nidPath  = "/home/yewang/test_data/nid.txt";
const char * g_dictPath = "/home/yewang/test_data/";


// 测试  getInstance 是否 会返回 NULL
TEST( DocIdManagerTest, Test_getInstance_NULL )
{
    for (int i = 0; i < FOR_NUM; ++i)
    {
        ASSERT_TRUE( (NULL != DocIdManager::getInstance()) ) << "null at index:" << i;
    }
}


// 测试  多次  getInstance 返回 的 指针 是否相同
TEST( DocIdManagerTest, Test_getInstance_DIFF )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    for (int i = 0; i < FOR_NUM; ++i)
    {
        ASSERT_TRUE( (dmgr == DocIdManager::getInstance()) ) << "diff at index:" << i;
    }
}



// 测试  多次  addNid 返回 的结果 是否 是顺序的
TEST( DocIdManagerTest, Test_addNid_Uniq )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    dmgr->reset();

    FILE * fp = fopen( g_nidPath, "rb" );

    if ( !fp )   ADD_FAILURE();

    ssize_t    read;
    char     * line = NULL;
    size_t     len  = 0;
    uint32_t   i    = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        ASSERT_EQ( i, dmgr->addNid( strtoll(line, NULL, 10) ) ) << "diff at index:" << i;
        ASSERT_EQ( strtoll(line, NULL, 10), dmgr->getNid( i ) ) << "diff at index:" << i;
        ++i;
    }

    ASSERT_EQ( i, dmgr->getDocIdCount() );

    fclose(fp);
    if ( line ) free(line);
}




// 测试  多次  getNid 返回 的结果 是否 是顺序的
TEST( DocIdManagerTest, Test_getNid_Uniq )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    FILE * fp = fopen( g_nidPath, "rb" );

    if ( !fp )   ADD_FAILURE();

    ssize_t    read;
    char     * line = NULL;
    size_t     len  = 0;
    uint32_t   i    = 0;

    while ( (read = getline(&line, &len, fp)) != -1 )
    {
        ASSERT_EQ( i, dmgr->getDocId( strtoll(line, NULL, 10) ) ) << "diff at index:" << line;
        ASSERT_EQ( strtoll(line, NULL, 10), dmgr->getNid( i ) ) << "diff at index:" << line;

        ++i;
    }

    if ( line ) free(line);
}






// 测试  添加了大量的nid后， 在删除掉， 然后重新获取
TEST( DocIdManagerTest, Test_add_Del_Get_Nid )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    dmgr->reset();

    FILE * fp = fopen( g_nidPath, "rb" );

    if ( !fp )   ADD_FAILURE();

    ssize_t    read;
    char     * line = NULL;
    size_t     len  = 0;
    uint32_t   i    = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        // 添加
        ASSERT_EQ( i, dmgr->addNid( strtoll(line, NULL, 10) ) ) << "diff at index:" << i;

        // 删除
        dmgr->delNid( strtoll(line, NULL, 10) );

        // 再取一次
        ASSERT_EQ( INVALID_NID, dmgr->getNid( i ) ) << "diff at index:" << i;

        // 判断是否删除
        ASSERT_TRUE ( dmgr->isDocIdDel( i ) );

        ++i;
    }

    ASSERT_EQ( i, dmgr->getDocIdCount() );
    ASSERT_EQ( i, dmgr->getDelCount() );

    fclose(fp);
    if ( line ) free(line);
}













// 添加重复的nid
TEST( DocIdManagerTest, Test_addNid_repeat )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    dmgr->reset();

    int64_t  nid = 10000;

    ASSERT_EQ( (uint32_t)0, dmgr->addNid( nid ) );

    // 不断的重复添加
    for (int i = 0; i < FOR_NUM; ++i)
    {
        ASSERT_EQ( INVALID_DOCID, dmgr->addNid( nid ) );
    }

    ASSERT_EQ( nid, dmgr->getNid( 0 ) );

    for (int i = 1; i < FOR_NUM; ++i)
    {
        ASSERT_EQ( INVALID_NID, dmgr->getNid( i ) );
    }
}





// 重复获取nid
TEST( DocIdManagerTest, Test_getNid_repeat )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    dmgr->reset();

    int64_t   nid   = 10000;
    uint32_t  docId = 0;

    ASSERT_EQ( docId, dmgr->addNid( nid ) );

    for (int i = 0; i < FOR_NUM; ++i)
    {
        ASSERT_EQ( nid, dmgr->getNid( docId ) );
    }

    for (int i = 1; i < FOR_NUM; ++i)
    {
        ASSERT_EQ( INVALID_NID, dmgr->getNid( i ) );
    }
}



// 添加后删除  然后检查
TEST( DocIdManagerTest, Test_add_del_check_get )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    dmgr->reset();

    int64_t   nid   = 10000;
    uint32_t  docId = 0;

    ASSERT_EQ( docId, dmgr->addNid( nid ) );

    dmgr->delDocId( docId );

    ASSERT_TRUE ( dmgr->isDocIdDel( docId ) );
    ASSERT_EQ( nid, dmgr->getNid( docId ) );
}



// 添加后删除  然后检查
TEST( DocIdManagerTest, Test_deleteMap )
{
    DocIdManager * dmgr = DocIdManager::getInstance();
    DocIdManager::DeleteMap * delMap = dmgr->getDeleteMap();

    dmgr->reset();

    int64_t   nid   = 10000;
    uint32_t  docId = 0;

    ASSERT_EQ( docId, dmgr->addNid( nid ) );

    dmgr->delDocId( docId );

    ASSERT_TRUE ( dmgr->isDocIdDel( docId ) );
    ASSERT_TRUE ( delMap->isDel( docId ) );

    ASSERT_EQ( nid, dmgr->getNid( docId ) );
}





TEST( DocIdManagerTest, Test_load )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    ASSERT_TRUE ( dmgr->load( g_dictPath ) );

    FILE * fp = fopen( g_nidPath , "rb");

    if ( !fp )   ADD_FAILURE();

    ssize_t    read;
    char     * line   = NULL;
    size_t     len    = 0;
    uint32_t   i      = 0;
    uint32_t   delNum = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        ASSERT_EQ( i, dmgr->getDocId( strtoll(line, NULL, 10) ) ) << "diff at index:" << i;

        if ( (i % 10) == 0 )
        {
            delNum++;
        }

        ++i;
    }

    ASSERT_EQ( i, dmgr->getDocIdCount() );
    ASSERT_EQ( delNum, dmgr->getDelCount() );

    fclose(fp);
    if ( line ) free(line);
}





TEST( DocIdManagerTest, Test_multi_load )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    ASSERT_TRUE ( dmgr->load( g_dictPath ) );
    ASSERT_TRUE ( dmgr->load( g_dictPath ) );
    ASSERT_TRUE ( dmgr->load( g_dictPath ) );
    ASSERT_TRUE ( dmgr->load( g_dictPath ) );

    FILE * fp = fopen( g_nidPath, "rb");

    if ( !fp )   ADD_FAILURE();

    ssize_t    read;
    char     * line   = NULL;
    size_t     len    = 0;
    uint32_t   i      = 0;
    uint32_t   delNum = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        ASSERT_EQ( i, dmgr->getDocId( strtoll(line, NULL, 10) ) ) << "diff at index:" << i;

        if ( (i % 10) == 0 )
        {
            delNum++;
        }

        ++i;
    }

    ASSERT_EQ( i, dmgr->getDocIdCount() );
    ASSERT_EQ( delNum, dmgr->getDelCount() );

    fclose(fp);
    if ( line ) free(line);
}





TEST( DocIdManagerTest, Test_load_modify )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    ASSERT_TRUE ( dmgr->load( g_dictPath ) );

    FILE * fp = fopen( g_nidPath , "rb");

    if ( !fp )   ADD_FAILURE();

    ssize_t    read;
    char     * line   = NULL;
    size_t     len    = 0;
    uint32_t   i      = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if ( (i % 10) == 0 )
        {
            dmgr->unDelDocId( i );
        }
        ++i;
    }

    ASSERT_EQ( i, dmgr->getDocIdCount() );
    ASSERT_EQ( (uint32_t)0, dmgr->getDelCount() );

    fclose(fp);
    if ( line ) free(line);

    int64_t   nid   = 1234;
    uint32_t  docId = i;

    ASSERT_EQ( docId, dmgr->addNid( nid ) );
    ASSERT_EQ( nid, dmgr->getNid( docId ) );
    ASSERT_EQ( i + 1, dmgr->getDocIdCount() );

    dmgr->delDocId( docId );

    ASSERT_TRUE ( dmgr->isDocIdDel( docId ) );
    ASSERT_EQ( nid, dmgr->getNid( docId ) );
    ASSERT_EQ( (uint32_t)1, dmgr->getDelCount() );
}






// 测试 将数据dump出去的功能是否正确
TEST( DocIdManagerTest, Test_dump )
{
    DocIdManager * dmgr = DocIdManager::getInstance();

    dmgr->reset();

    FILE * fp = fopen( g_nidPath , "rb");

    if ( !fp )   ADD_FAILURE();

    ssize_t    read;
    char     * line   = NULL;
    size_t     len    = 0;
    uint32_t   i      = 0;
    uint32_t   delNum = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        ASSERT_EQ( i, dmgr->addNid( strtoll(line, NULL, 10) ) ) << "diff at index:" << i;

        if ( (i % 10) == 0 )
        {
            dmgr->delDocId( i );
            delNum++;
        }

        ++i;
    }

    ASSERT_EQ( i, dmgr->getDocIdCount() );
    ASSERT_EQ( delNum, dmgr->getDelCount() );

    fclose(fp);
    if ( line ) free(line);

    //dump输出数据
    ASSERT_TRUE ( dmgr->dump( g_dictPath ) );
}







// 主测试入口
int main(int argc, char * argv[])
{
    alog::Configurator::configureRootLogger();

     ::testing::InitGoogleTest(&argc, argv);

     return RUN_ALL_TESTS();
}
