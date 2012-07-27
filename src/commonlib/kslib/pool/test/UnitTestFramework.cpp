#include "UnitTestFramework.h"
#include <iostream>

UnitTestFramework::UnitTestFramework()
{
}

UnitTestFramework::~UnitTestFramework()
{
    for(std::vector<UnitTestCase*>::iterator iter = m_vTestCases.begin(); iter != m_vTestCases.end();iter++)
    {
        delete *iter;
    }
    m_vTestCases.clear();
}

void UnitTestFramework::run()
{
    UnitTestCase* pTestCase;
    bool bOk;
    size_t nTotal = 0,nFailed = 0;
    for(std::vector<UnitTestCase*>::iterator iter = m_vTestCases.begin(); iter != m_vTestCases.end();iter++)
    {
        pTestCase = *iter;
        std::cout << "->Check:" << pTestCase->getName() << "...";
        try
        {
            pTestCase->run();
        }
        catch(...)
        {
            std::cout << "[FATAL ERROR]";
            pTestCase->setFailed();
        }
        
        nFailed += pTestCase->getFailed();
        if(pTestCase->getFailed() == 0)
            std::cout <<"[OK]" << std::endl;
        else 
            std::cout << "[" << pTestCase->getFailed() << " FAILED]" << std::endl;
        nTotal++;
    }
    std::cout << "==================================" << std::endl;
    std::cout << "Total test:" << nTotal << ". Success: " << nTotal - nFailed << ". Failed: " << nFailed << std::endl;
}

void UnitTestFramework::addCase(UnitTestCase* pTestCase)
{
    m_vTestCases.push_back(pTestCase);
}
