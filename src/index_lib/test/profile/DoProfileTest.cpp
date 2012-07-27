#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include "index_lib/IndexLib.h"

int main(int argc, char* argv[])
{
    alog::Configurator::configureRootLogger();
    // Get the top level suite from the registry
    CppUnit::Test *suite_hashTable      = CppUnit::TestFactoryRegistry::getRegistry("hash_table").makeTest();
    CppUnit::Test *suite_mmapFile       = CppUnit::TestFactoryRegistry::getRegistry("mmap_file").makeTest();
    CppUnit::Test *suite_encodeFile     = CppUnit::TestFactoryRegistry::getRegistry("encode_file").makeTest();
    CppUnit::Test *suite_docAccessor    = CppUnit::TestFactoryRegistry::getRegistry("doc_accessor").makeTest();
    CppUnit::Test *suite_profileManager = CppUnit::TestFactoryRegistry::getRegistry("profile_manager").makeTest();

    // Adds the test to the list of test to run
    CppUnit::TextUi::TestRunner runner;
    runner.addTest(suite_hashTable);
    runner.addTest(suite_mmapFile);
    runner.addTest(suite_encodeFile);
    runner.addTest(suite_profileManager);
    runner.addTest(suite_docAccessor);

    // Change the default outputter to a compiler error format outputter
    runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(), std::cerr ) );

    // Run the tests.
    bool wasSucessful = runner.run();

    // Return error code 1 if the one of test failed.
    return wasSucessful ? 0 : 1;
}

