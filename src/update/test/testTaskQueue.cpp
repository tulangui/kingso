#include "testTaskQueue.h"
#include "common/TaskQueue.h"

CPPUNIT_TEST_SUITE_REGISTRATION(testTaskQueue);

class STask : public update::Session {
    public:
        STask(uint64_t seq) {_level = 1; _seq=seq; }
        ~STask() {}
    private:
};

class DTask : public update::Session {
    public:
        DTask(uint64_t seq) {_level = 0;_seq = seq; }
        ~DTask() {}
    private:
};

testTaskQueue::testTaskQueue()
{
}

testTaskQueue::~testTaskQueue()
{
}

void testTaskQueue::testPushPop()
{
    update::TaskQueue que;
    CPPUNIT_ASSERT(que.init(10240) == 0);

    DTask d1(1), d2(2), d3(3);
    STask s1(1), s2(2), s3(3);

    que.enqueue(&s1);
    que.enqueue(&s2);
    que.enqueue(&s3);
    que.enqueue(&d1);
    que.enqueue(&d2);
    que.enqueue(&d3);

    update::Session* task = NULL;

    CPPUNIT_ASSERT(que.dequeue(&task) == 0);
    task->show(_message);
    CPPUNIT_ASSERT_MESSAGE(_message, task == &d1);

    CPPUNIT_ASSERT(que.dequeue(&task) == 0 && task == &s1);
    CPPUNIT_ASSERT(que.dequeue(&task) == 0 && task == &d2);
    CPPUNIT_ASSERT(que.dequeue(&task) == 0 && task == &s2);
    CPPUNIT_ASSERT(que.dequeue(&task) == 0 && task == &d3);
    CPPUNIT_ASSERT(que.dequeue(&task) == 0 && task == &s3);
}


