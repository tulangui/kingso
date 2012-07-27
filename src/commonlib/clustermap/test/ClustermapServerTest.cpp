#include <cppunit/extensions/HelperMacros.h>
#include "ClustermapServerTool.h"
#include "NodeinfoTool.h"

namespace clustermap {

class ClustermapServerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ClustermapServerTest);
    CPPUNIT_TEST(testStartMasterServer);
    CPPUNIT_TEST(testStartSlaveServer);
    CPPUNIT_TEST(testNodeinfoWall);
    CPPUNIT_TEST_SUITE_END();

    public:
    ClustermapServerTest();
    ~ClustermapServerTest();
    public:
    void setUp();
    void tearDown();
    void testStartMasterServer();
    void testStartSlaveServer();
    void testNodeinfoWall();
    public:
        ClustermapServerTool _cstool;
    NodeinfoTool _nitool;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ClustermapServerTest);

ClustermapServerTest::ClustermapServerTest()
{
}

ClustermapServerTest::~ClustermapServerTest()
{
}

void ClustermapServerTest::setUp()
{
}

void ClustermapServerTest::tearDown()
{
    _cstool.finishMasterServer();
}

void ClustermapServerTest::testStartMasterServer()
{
    CPPUNIT_ASSERT(_cstool.startMasterServer());
}

void ClustermapServerTest::testStartSlaveServer()
{
    CPPUNIT_ASSERT(_cstool.startSlaveServer());
}

void ClustermapServerTest::testNodeinfoWall()
{
    CPPUNIT_ASSERT(_cstool.startMasterServer());

    CPPUNIT_ASSERT(_nitool.NodeinfoWall());
    CPPUNIT_ASSERT(_nitool.hasClusterName("b1"));
    CPPUNIT_ASSERT(!_nitool.hasClusterName("b100"));
}

}
