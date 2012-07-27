/******************************************************************
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : DocIdManger.cpp 的测试程序， 使用 gtest框架
 *
 ******************************************************************/

#include <gtest/gtest.h>
#include "DocIdManager.h"


// EXPECT_* 失败时，案例继续往下执行
// ASSERT_* 失败时，直接在当前函数中返回，当前函数中ASSERT_*后面的语句将不会执行


// TEST宏, 有两个参数，解释为：[TestCaseName, TestName]
TEST(casename, testname1)
{
    EXPECT_EQ(2, add(1, 1));
    EXPECT_EQ(3, add(1, 1));
}


TEST(casename, testname2)
{
    for (int i = 0; i < x.size(); ++i)
    {
        EXPECT_EQ(x[i], y[i]);
        EXPECT_EQ(x[i], y[i]) << "Vectors x and y differ at index " << i;
    }
}


// 判断bool值
TEST(casename, testname3)
{
    ASSERT_TRUE( add(1, 1) );
    EXPECT_TRUE( add(1, 1) );
}


// 判断字符串
TEST(casename, testname4)
{
    EXPECT_STREQ(expected_str, actual_str);
    EXPECT_STRNE(expected_str, actual_str);
}


// 直接返回成功 失败
TEST(casename, testname5)
{
    SUCCEED();
    ADD_FAILURE();
}




testing::AssertionResult AssertFoo(const char* m_expr, const char* n_expr, const char* k_expr, int m, int n, int k) {
    if (Foo(m, n) == k)
        return testing::AssertionSuccess();
    testing::Message msg;
    msg << m_expr << " 和 " << n_expr << " 的最大公约数应该是：" << Foo(m, n) << " 而不是：" << k_expr;
    return testing::AssertionFailure(msg);
}

TEST(AssertFooTest, HandleFail)
{
    EXPECT_PRED_FORMAT3(AssertFoo, 3, 6, 2);
}




// 检查浮点数
TEST(casename, testname5)
{
    EXPECT_FLOAT_EQ(expected, actual);
    EXPECT_DOUBLE_EQ(expected, actual);
}


// 死亡测试
TEST(FooDeathTest, Demo)
{
    EXPECT_DEATH(Foo(), "core .....");
}



int main(int argc, char * argv[])
{
     ::testing::InitGoogleTest(&argc, argv);

     return RUN_ALL_TESTS();
}
