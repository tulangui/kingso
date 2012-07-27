#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <gtest/gtest.h>


#include "index_lib/varint.h"


using namespace index_lib;



TEST( VarintTest, Test_encode_1_byte1 )
{
    uint8_t  buf[5] = {0};
    uint32_t value  = 1;

    varint_encode_uint32( value, buf);

    EXPECT_STREQ( "\001", (const char*)buf );
}


TEST( VarintTest, Test_decode_1_byte1 )
{
    uint8_t  buf[5] = "\001";
    uint32_t value  = 0;

    varint_decode_uint32(buf, &value);

    EXPECT_EQ( (uint32_t)1, value );
}



TEST( VarintTest, Test_encode_1_byte2 )
{
    uint8_t  buf[5] = {0};
    uint32_t value  = 55;

    varint_encode_uint32( value, buf);

    EXPECT_STREQ( "7", (const char*)buf );
}


TEST( VarintTest, Test_decode_1_byte2 )
{
    uint8_t  buf[5] = "7";
    uint32_t value  = 0;

    varint_decode_uint32(buf, &value);

    EXPECT_EQ( (uint32_t)55, value );
}



TEST( VarintTest, Test_encode_decode )
{
    uint8_t  buf[5]    = {0};
    uint32_t srcValue  = 0;
    uint32_t destValue = 0;

    for ( srcValue = UINT_MAX - 100; srcValue < UINT_MAX; srcValue++ )
    {
        memset(buf, 0, sizeof(buf));
        destValue = 0;

        varint_encode_uint32( srcValue, buf);
        varint_decode_uint32(buf, &destValue);

        EXPECT_EQ( srcValue, destValue );
    }
}




TEST( VarintTest, Test_encode_decode_arr )
{
    uint8_t  buf[1024] = {0};
    uint32_t arr[100]  = {0};
    uint32_t destValue = 0;

    uint8_t * target = buf;

    for ( uint32_t i = 0; i < (sizeof(arr)/sizeof(uint32_t)); i++ )
    {
        srand(i);
        arr[i] = i + 1; //(uint32_t)rand();
    }

    for ( uint32_t i = 0; i < (sizeof(arr)/sizeof(uint32_t)); i++ )
    {
        target = varint_encode_uint32( arr[i], target);
    }

    target = buf;

    for ( uint32_t i = 0; i < (sizeof(arr)/sizeof(uint32_t)); i++ )
    {
        target = (uint8_t *)varint_decode_uint32(target, &destValue);
        EXPECT_EQ( arr[i], destValue );
    }
}


//==========================================================================

TEST( GroupVarintTest, Test_encode_decode )
{
    uint8_t  buf[17] = {0};
    uint32_t valueArr[4] = {1,2,3,4};
    uint32_t tempArr[4]  = {0};

    uint8_t * tmp = group_varint_encode_uint32( valueArr, buf);

    EXPECT_EQ( 5, (tmp - buf) );

    EXPECT_EQ( 0, buf[0] );
    EXPECT_EQ( 1, buf[1] );
    EXPECT_EQ( 2, buf[2] );
    EXPECT_EQ( 3, buf[3] );
    EXPECT_EQ( 4, buf[4] );

    const uint8_t * tmp2 = group_varint_decode_uint32 ( buf , tempArr);

    EXPECT_EQ( 5, (tmp2 - buf) );

    EXPECT_EQ( (uint32_t)1, tempArr[0] );
    EXPECT_EQ( (uint32_t)2, tempArr[1] );
    EXPECT_EQ( (uint32_t)3, tempArr[2] );
    EXPECT_EQ( (uint32_t)4, tempArr[3] );
}


TEST( GroupVarintTest, Test_encode_decode_3byte )
{
    uint8_t  buf[17] = {0};
    uint32_t valueArr[4] = {5005850,5005850,5005850,5005850};
    uint32_t tempArr[4]  = {0};

    group_varint_encode_uint32( valueArr, buf);

    const uint8_t * tmp2 = group_varint_decode_uint32 ( buf , tempArr);

    EXPECT_EQ( 13, (tmp2 - buf) );

    ASSERT_EQ( (uint32_t)5005850, tempArr[0] );
    ASSERT_EQ( (uint32_t)5005850, tempArr[1] );
    ASSERT_EQ( (uint32_t)5005850, tempArr[2] );
    ASSERT_EQ( (uint32_t)5005850, tempArr[3] );
}




TEST( GroupVarintTest, Test_group_encode_decode_arr )
{
    uint8_t  buf[102400] = {0};
    uint32_t arr[10000]  = {0};
    uint32_t dest[10000] = {0};

    for ( uint32_t i = 0; i < 10000; i++ )
    {
        srand(i);
        arr[i] = zigZag_encode32(rand());
    }

    uint8_t * target = buf;
    for ( uint32_t i = 0; i < 2500; i++ )
    {
        target = group_varint_encode_uint32( &arr[ i * 4 ], target);
    }

    const uint8_t * tmp = buf;
    for ( uint32_t i = 0; i < 2500; i++ )
    {
        tmp = group_varint_decode_uint32(tmp, &dest[ i * 4 ]);

        for ( uint32_t j = 0; j < 4; j++ )
        {
            ASSERT_EQ( arr[ i * 4 + j], dest[ i * 4 + j] );
        }
    }


}






// 主测试入口
int main(int argc, char * argv[])
{
     ::testing::InitGoogleTest(&argc, argv);

     return RUN_ALL_TESTS();
}
