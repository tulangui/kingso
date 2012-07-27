#ifndef __UNITTESTFRAMEWORK_H
#define __UNITTESTFRAMEWORK_H

#include <vector>
#include <string>

class UnitTestCase
{
public:
    UnitTestCase(const char* szTestName)
        : m_sName(szTestName)
        , m_nFailed(0)
    {}
    
    virtual ~UnitTestCase(){}
public:
    /**
     * run test case
     */
    virtual void run() = 0;

public:
    /** get test name */
    const std::string& getName(){return m_sName;} 

    /** get/set failed count*/
    const size_t getFailed(){return m_nFailed;}
    void setFailed(){m_nFailed++;}
protected:
    std::string m_sName;
    size_t m_nFailed;
};

class UnitTestFramework
{
    UnitTestFramework();
public:
    ~UnitTestFramework();
public:
    static UnitTestFramework& getInstance()
    {
        static UnitTestFramework TEST_RUNNER;
        return TEST_RUNNER;
    }
public:
    /**
     * run test case
     */
    void run();

    /**
     * add a test case 
     * @param pTestCase test case object
     */
    void addCase(UnitTestCase* pTestCase);

protected:
    std::vector<UnitTestCase*> m_vTestCases;
};

#define TEST_ASSERT(b) if(!(b) ) this->setFailed()

#endif

